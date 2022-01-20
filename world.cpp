#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>
#include <random>

#include "world.h"
#include "inventory.h"
#include "types.h"

using namespace WorldTypes;


void WorldBlock::update()
{
}

Loot WorldBlock::destroy()
{
	return Loot{};
}

TextureFace WorldBlock::texture()
{
	switch(blockType)
	{
		case Block::dirt:
			return info.grassy ? TextureFace{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 1}, {0, 2}}
			: TextureFace{{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}};
			
		case Block::stone:
			return TextureFace{{1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}};
			
		case Block::sand:
			return TextureFace{{2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}};
			
		case Block::log:
			return TextureFace{{3, 0}, {3, 0}, {3, 0}, {3, 0}, {3, 1}, {3, 1}};
			
		case Block::leaf:
			return TextureFace{{4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}};
			
		case Block::cactus:
			return TextureFace{{5, 0}, {5, 0}, {5, 0}, {5, 0}, {5, 1}, {5, 1}};
		
		default:
			return TextureFace{};
	}
}

bool WorldBlock::transparent()
{
	switch(blockType)
	{
		case Block::leaf:
		case Block::air:
			return true;
		
		default:
			return false;
	}
}


WorldChunk::WorldChunk(WorldGenerator* wGen, Vec3d<int> pos) : _wGen(wGen), _position(pos)
{
	_textureWidth = wGen->_texAtlas._width;
	_textureHeight = wGen->_texAtlas._height;
	_textureHBlocks = wGen->_texAtlas._horizontalBlocks;
	_textureVBlocks = wGen->_texAtlas._verticalBlocks;
	_textureOffset = wGen->_texAtlas._texOffset;

	_modelName = model_name(_position);
}

bool WorldChunk::has_transparent()
{
	int blockIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{
				if(_chunkBlocks[blockIndex].transparent())
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool WorldChunk::check_empty()
{
	int blockIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{
				if(_chunkBlocks[blockIndex].blockType!=Block::air)
				{
					return false;
				}
			}
		}
	}
	
	return true;
}


void WorldChunk::chunk_gen()
{	
	if(_position.y>_wGen->genHeight)
	{
		_empty = true;
		return;
	}
	
	_empty = false;
	
	bool overground = _position.y>=0;
	
	if(!overground)
	{
		_chunkBlocks.fill(WorldBlock{Block::stone});
		
		update_states();
		
		_chunkModel = YandereModel();
		apply_model();
		
		return;
	}
	
	std::array<float, chunkSize*chunkSize> noiseArr = _wGen->generate(_position);
	std::array<ClimatePoint, chunkSize*chunkSize> climateArr = _wGen->generate_climate(_position);
	
	
	int blockIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{
				int mapsIndex = x*chunkSize+z;
				float currNoise = noiseArr[mapsIndex]*chunkSize;
			
				if(_position.y*chunkSize+y<currNoise)
				{
					switch(_wGen->get_biome(climateArr[mapsIndex].temperature, climateArr[mapsIndex].humidity))
					{
						case Biome::desert:
						{
							_chunkBlocks[blockIndex] = WorldBlock{Block::sand};
							break;
						}
						
						default:
						case Biome::forest:
						{
							bool currGrass = (_position.y*chunkSize+y+1)>=currNoise;
							_chunkBlocks[blockIndex] = WorldBlock{Block::dirt, BlockInfo{currGrass}};
							break;
						}
					}
				} else
				{
					_chunkBlocks[blockIndex] = WorldBlock{Block::air};
				}
			}
		}
	}
	
	_wGen->gen_plants(*this, climateArr);
	
	update_states();
	update_mesh();
}

void WorldChunk::update_states()
{
	if(_empty)
		return;

	std::for_each(std::execution::par_unseq, _chunkBlocks.begin(), _chunkBlocks.end(), [](WorldBlock& block)
	{
			block.update();
	});
}

void WorldChunk::shared_place(Vec3d<int> position, WorldBlock block)
{
	if(position.x<0 || position.y<0 || position.z<0
	|| position.x>(chunkSize-1) || position.y>(chunkSize-1) || position.z>(chunkSize-1))
	{
		//outside of current chunk
		Vec3d<int> placeChunk = active_chunk(position);
		
		
	} else
	{
		this->block(position) = block;
	}
}

Vec3d<int> WorldChunk::active_chunk(Vec3d<int> pos)
{
	Vec3d<int> retPos = Vec3d<int>{0, 0, 0};
	
	if(pos.x<0)
	{
		retPos.x = (pos.x-chunkSize)/chunkSize;
	}
	
	if(pos.x>0)
	{
		retPos.x = pos.x/chunkSize;
	}
			
	if(pos.y<0)
	{
		retPos.y = (pos.y-chunkSize)/chunkSize;
	}
			
	if(pos.y>0)
	{
		retPos.y = pos.y/chunkSize;
	}
			
	if(pos.z<0)
	{
		retPos.z = (pos.z-chunkSize)/chunkSize;
	}
			
	if(pos.z>0)
	{
		retPos.z = pos.z/chunkSize;
	}
	
	return retPos;
}

Vec3d<int> WorldChunk::active_chunk(Vec3d<float> pos)
{
	Vec3d<int> retPos = Vec3d<int>{0, 0, 0};
	
	if(pos.x<0)
	{
		retPos.x = (static_cast<int>(pos.x)-chunkSize)/chunkSize;
	}
	
	if(pos.x>0)
	{
		retPos.x = (static_cast<int>(pos.x))/chunkSize;
	}
			
	if(pos.y<0)
	{
		retPos.y = (static_cast<int>(pos.y)-chunkSize)/chunkSize;
	}
			
	if(pos.y>0)
	{
		retPos.y = (static_cast<int>(pos.y))/chunkSize;
	}
			
	if(pos.z<0)
	{
		retPos.z = (static_cast<int>(pos.z)-chunkSize)/chunkSize;
	}
			
	if(pos.z>0)
	{
		retPos.z = (static_cast<int>(pos.z))/chunkSize;
	}
	
	return retPos;
}

void WorldChunk::update_mesh()
{
	if(_empty)
		return;

	_chunkModel = YandereModel();
		
	_indexOffset = 0;
		
	int airAmount = 0;
	int blockIndex = 0;
		
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{	
				if(_chunkBlocks[blockIndex].blockType!=Block::air)
				{
					if(z!=(chunkSize-1) && _chunkBlocks[blockIndex+1].transparent())
					{
						a_forwardFace({x, y, z});
					}
						
					if(z!=0 && _chunkBlocks[blockIndex-1].transparent())
					{
						a_backFace({x, y, z});
					}
					
					if(y!=(chunkSize-1) && _chunkBlocks[blockIndex+chunkSize].transparent())
					{
						a_upFace({x, y, z});
					}
					
					if(y!=0 && _chunkBlocks[blockIndex-chunkSize].transparent())
					{
						a_downFace({x, y, z});
					}
					
					if(x!=(chunkSize-1) && _chunkBlocks[blockIndex+chunkSize*chunkSize].transparent())
					{
						a_rightFace({x, y, z});
					}
					
					if(x!=0 && _chunkBlocks[blockIndex-chunkSize*chunkSize].transparent())
					{
						a_leftFace({x, y, z});
					}
				} else
				{
					++airAmount;
				}
			}
		}
	}
	
	if(airAmount==chunkSize)
	{
		_empty = true;
		return;
	}
	
	apply_model();
}

void WorldChunk::apply_model()
{
	if(!_empty)
		_wGen->_init->_modelMap[_modelName] = _chunkModel;
}

void WorldChunk::remove_model()
{
	if(!_empty)
		_wGen->_init->_modelMap.erase(_modelName);
		
	_empty = true;
}

void WorldChunk::update_wall(Direction wall, WorldChunk* checkChunk)
{
	if(_empty || checkChunk->empty())
		return;
		

	switch(wall)
	{
		case Direction::right:
		{
			int startingIndex = (chunkSize-1)*chunkSize*chunkSize;
			int blockIndex = startingIndex;
		
			for(int y = 0; y < chunkSize; ++y)
			{
				for(int z = 0; z < chunkSize; ++z, ++blockIndex)
				{
					if(!_chunkBlocks[blockIndex].transparent() && checkChunk->_chunkBlocks[blockIndex-startingIndex].transparent())
					{
						a_rightFace({chunkSize-1, y, z});
					}
				}
			}
			break;
		}
		
		case Direction::left:
		{
			int startingIndex = (chunkSize-1)*chunkSize*chunkSize;
			int blockIndex = startingIndex;
		
			for(int y = 0; y < chunkSize; ++y)
			{
				for(int z = 0; z < chunkSize; ++z, ++blockIndex)
				{
					if(!_chunkBlocks[blockIndex-startingIndex].transparent() && checkChunk->_chunkBlocks[blockIndex].transparent())
					{
						a_leftFace({0, y, z});
					}
				}
			}
			break;
		}
		
		case Direction::up:
		{
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int z = 0; z < chunkSize; ++z)
				{
					if(!block({x, chunkSize-1, z}).transparent() && checkChunk->block({x, 0, z}).transparent())
					{
						a_upFace({x, chunkSize-1, z});
					}
				}
			}
			break;
		}
		
		case Direction::down:
		{
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int z = 0; z < chunkSize; ++z)
				{
					if(!block({x, 0, z}).transparent() && checkChunk->block({x, chunkSize-1, z}).transparent())
					{
						a_downFace({x, 0, z});
					}
				}
			}
			break;
		}
		
		case Direction::forward:
		{
			int blockIndex = 0;
		
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int y = 0; y < chunkSize; ++y, blockIndex+=chunkSize)
				{
					if(!_chunkBlocks[blockIndex+chunkSize-1].transparent() && checkChunk->_chunkBlocks[blockIndex].transparent())
					{
						a_forwardFace({x, y, chunkSize-1});
					}
				}
			}
			break;
		}
		
		case Direction::back:
		{
			int blockIndex = 0;
		
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int y = 0; y < chunkSize; ++y, blockIndex+=chunkSize)
				{
					if(!_chunkBlocks[blockIndex].transparent() && checkChunk->_chunkBlocks[blockIndex+chunkSize-1].transparent())
					{
						a_backFace({x, y, 0});
					}
				}
			}
			break;
		}
	}
}


void WorldChunk::a_forwardFace(Vec3d<int> pos)
{
	//i cant write any better code for these, it literally HAS to be hardcoded :/
	TexPos texturePos = block(pos).texture().forward;

	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};

	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_backFace(Vec3d<int> pos)
{
	TexPos texturePos = block(pos).texture().back;
	
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_leftFace(Vec3d<int> pos)
{
	TexPos texturePos = block(pos).texture().left;
	
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_rightFace(Vec3d<int> pos)
{
	TexPos texturePos = block(pos).texture().right;
	
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});	
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_upFace(Vec3d<int> pos)
{
	TexPos texturePos = block(pos).texture().up;
	
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_downFace(Vec3d<int> pos)
{
	TexPos texturePos = block(pos).texture().down;
	
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}

Vec3d<int> WorldChunk::closest_block(Vec3d<float> pos)
{
	return {static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)), static_cast<int>(std::round(pos.z))};
}

WorldBlock& WorldChunk::block(Vec3d<int> pos)
{
	return _chunkBlocks[pos.x*chunkSize*chunkSize+pos.y*chunkSize+pos.z];
}

bool WorldChunk::empty()
{
	return _empty || _wGen==nullptr;
}

Vec3d<int> WorldChunk::position()
{
	return _position;
}

std::string WorldChunk::model_name(Vec3d<int> pos)
{
	std::string chunkModelName = "!cHUNK" + std::to_string(pos.x);
	chunkModelName += '_';
	chunkModelName += std::to_string(pos.y);
	chunkModelName += '_';
	chunkModelName += std::to_string(pos.z);
	
	return chunkModelName;
}
