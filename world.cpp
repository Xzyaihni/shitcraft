#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>
#include <random>

#include "world.h"
#include "inventory.h"
#include "types.h"

using namespace WorldTypes;


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
	assert(_wGen!=nullptr);	
	
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

void WorldChunk::shared_place(Vec3d<int> position, WorldBlock block, bool replace)
{
	if(position.x<0 || position.y<0 || position.z<0
	|| position.x>(chunkSize-1) || position.y>(chunkSize-1) || position.z>(chunkSize-1))
	{
		//outside of current chunk
		Vec3d<int> placeChunk = active_chunk(position);
		Vec3d<int> newPos = position-placeChunk*chunkSize;
		
		Direction currSide;
		
		bool found = false;
		
		if(!found && position.x<0)
			currSide = Direction::right;
			
		if(!found && position.y<0)
			currSide = Direction::up;
			
		if(!found && position.z<0)
			currSide = Direction::forward;
			
		if(!found && position.x>(chunkSize-1))
			currSide = Direction::left;
			
		if(!found && position.y>(chunkSize-1))
			currSide = Direction::down;
			
		if(!found && position.z>(chunkSize-1))
			currSide = Direction::back;
		
		_wGen->place_in_chunk(currSide, _position+placeChunk, newPos, block, replace);
	} else
	{
		if(replace || this->block(position).blockType==Block::air)
		{
			this->block(position) = block;
		}
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
					update_block_walls({x, y, z}, blockIndex);
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
}


void WorldChunk::update_block_walls(Vec3d<int> pos)
{
	update_block_walls(pos, pos.x*chunkSize*chunkSize+pos.y*chunkSize+pos.z);
}

void WorldChunk::update_block_walls(Vec3d<int> pos, int index)
{
	//if both the current and the check block are the same, don't draw the side (makes stuff like leaves connect into a single thing)
	int bCurr = _chunkBlocks[index].blockType;
	
	bool nextTzp = pos.z!=(chunkSize-1) ? _chunkBlocks[index+1].transparent() : false;
	bool nextTzn = pos.z!=0 ? _chunkBlocks[index-1].transparent() : false;
	
	bool nextTyp = pos.y!=(chunkSize-1) ? _chunkBlocks[index+chunkSize].transparent() : false;
	bool nextTyn = pos.y!=0 ? _chunkBlocks[index-chunkSize].transparent() : false;
	
	bool nextTxp = pos.x!=(chunkSize-1) ? _chunkBlocks[index+chunkSize*chunkSize].transparent() : false;
	bool nextTxn = pos.x!=0 ? _chunkBlocks[index-chunkSize*chunkSize].transparent() : false;
	
	
	if(nextTzp && !(bCurr == _chunkBlocks[index+1].blockType))
	{
		a_forwardFace(pos, _chunkBlocks[index].texture().forward);
	}
	
	if(nextTzn && !(bCurr == _chunkBlocks[index-1].blockType))
	{
		a_backFace(pos, _chunkBlocks[index].texture().back);
	}

	if(nextTyp && !(bCurr == _chunkBlocks[index+chunkSize].blockType))
	{
		a_upFace(pos, _chunkBlocks[index].texture().up);
	}

	if(nextTyn && !(bCurr == _chunkBlocks[index-chunkSize].blockType))
	{
		a_downFace(pos, _chunkBlocks[index].texture().down);
	}

	if(nextTxp && !(bCurr == _chunkBlocks[index+chunkSize*chunkSize].blockType))
	{
		a_rightFace(pos, _chunkBlocks[index].texture().right);
	}

	if(nextTxn && !(bCurr == _chunkBlocks[index-chunkSize*chunkSize].blockType))
	{
		a_leftFace(pos, _chunkBlocks[index].texture().left);
	}
}

void WorldChunk::apply_model()
{
	assert(_wGen!=nullptr);

	if(!_empty)
		_wGen->_init->_modelMap[_modelName] = _chunkModel;
}

void WorldChunk::remove_model()
{
	assert(_wGen!=nullptr);
	
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
					if(_chunkBlocks[blockIndex].blockType!=Block::air)
					{
						if(checkChunk->_chunkBlocks[blockIndex-startingIndex].transparent()
						&& (_chunkBlocks[blockIndex].blockType != checkChunk->_chunkBlocks[blockIndex-startingIndex].blockType))
						{
							a_rightFace({chunkSize-1, y, z}, _chunkBlocks[blockIndex].texture().right);
						}
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
					if(_chunkBlocks[blockIndex-startingIndex].blockType!=Block::air)
					{
						if(checkChunk->_chunkBlocks[blockIndex].transparent()
						&& (_chunkBlocks[blockIndex-startingIndex].blockType != checkChunk->_chunkBlocks[blockIndex].blockType))
						{
							a_leftFace({0, y, z}, _chunkBlocks[blockIndex-startingIndex].texture().left);
						}
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
					WorldBlock* blockCurr = &block({x, chunkSize-1, z});
					WorldBlock* blockCheck = &checkChunk->block({x, 0, z});
					
					if(blockCurr->blockType!=Block::air)
					{
						if(blockCheck->transparent()
						&& (blockCurr->blockType != blockCheck->blockType))
						{
							a_upFace({x, chunkSize-1, z}, blockCurr->texture().up);
						}
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
					WorldBlock* blockCurr = &block({x, 0, z});
					WorldBlock* blockCheck = &checkChunk->block({x, chunkSize-1, z});
				
					if(blockCurr->blockType!=Block::air)
					{
						if(blockCheck->transparent()
						&& (blockCurr->blockType != blockCheck->blockType))
						{
							a_downFace({x, 0, z}, blockCurr->texture().down);
						}
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
					if(_chunkBlocks[blockIndex+chunkSize-1].blockType!=Block::air)
					{
						if(checkChunk->_chunkBlocks[blockIndex].transparent()
						&& (_chunkBlocks[blockIndex+chunkSize-1].blockType != checkChunk->_chunkBlocks[blockIndex].blockType))
						{
							a_forwardFace({x, y, chunkSize-1}, _chunkBlocks[blockIndex+chunkSize-1].texture().forward);
						}
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
					if(_chunkBlocks[blockIndex].blockType!=Block::air)
					{
						if(checkChunk->_chunkBlocks[blockIndex+chunkSize-1].transparent()
						&& (_chunkBlocks[blockIndex].blockType != checkChunk->_chunkBlocks[blockIndex+chunkSize-1].blockType))
						{
							a_backFace({x, y, 0}, _chunkBlocks[blockIndex].texture().back);
						}
					}
				}
			}
			break;
		}
	}
}


void WorldChunk::a_forwardFace(Vec3d<int> pos, TexPos texturePos)
{
	//i cant write any better code for these, it literally HAS to be hardcoded :/
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};

	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_backFace(Vec3d<int> pos, TexPos texturePos)
{
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_leftFace(Vec3d<int> pos, TexPos texturePos)
{
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_rightFace(Vec3d<int> pos, TexPos texturePos)
{
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});	
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_upFace(Vec3d<int> pos, TexPos texturePos)
{
	Vec3d<float> posU = {pos.x*blockModelSize, pos.y*blockModelSize, pos.z*blockModelSize};
	
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_downFace(Vec3d<int> pos, TexPos texturePos)
{
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

void WorldChunk::set_empty(bool state)
{
	_empty = state;
}

bool WorldChunk::empty()
{
	return _empty;
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
