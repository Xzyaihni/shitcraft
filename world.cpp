#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>

#include <random>

#include "world.h"
#include "inventory.h"
#include "types.h"

using namespace WorldTypes;


WorldChunk::WorldChunk(WorldGenerator* wGen, Vec3d<int> pos) : _wGen(wGen), _position(pos), _chunkModel()
{
	_textureWidth = wGen->_texAtlas._width;
	_textureHeight = wGen->_texAtlas._height;
	_textureHBlocks = wGen->_texAtlas._horizontalBlocks;
	_textureVBlocks = wGen->_texAtlas._verticalBlocks;
	_textureOffset = wGen->_texAtlas._texOffset;
}

void WorldChunk::create_mesh()
{
	if(!_modelCreated)
	{
		_modelID = _wGen->_init->add_model(_chunkModel);
		_modelCreated = true;
	}
}

unsigned WorldChunk::modelID()
{
	return _modelID;
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


void WorldChunk::update_states()
{
	if(_empty)
		return;

	std::for_each(std::execution::par_unseq, _chunkBlocks.begin(), _chunkBlocks.end(), [](WorldBlock& block)
	{
			block.update();
	});
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
	_chunkModel = YandereModel();

	if(_empty)
	{
		apply_model();
		return;
	}
		
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
	
	apply_model();
	
	if(airAmount==chunkSize*chunkSize*chunkSize)
	{
		_empty = true;
		return;
	}
}


void WorldChunk::update_block_walls(const Vec3d<int> pos) const noexcept
{
	update_block_walls(pos, pos.x*chunkSize*chunkSize+pos.y*chunkSize+pos.z);
}

void WorldChunk::update_block_walls(const Vec3d<int> pos, const int index) const noexcept
{
	//if both the current and the check block are the same, don't draw the side (makes stuff like leaves connect into a single thing)
	const WorldBlock& currBlock = _chunkBlocks[index];
	int bCurr = currBlock.blockType;
	
	const TextureFace& currText = currBlock.texture();
	
	const Vec3d<float> posU = Vec3dCVT<float>(pos)*blockModelSize;
	
	if(pos.z!=(chunkSize-1) && _chunkBlocks[index+1].transparent() && (bCurr != _chunkBlocks[index+1].blockType))
		a_forwardFace(posU, currText.forward);
	
	if(pos.z!=0 && _chunkBlocks[index-1].transparent() && (bCurr != _chunkBlocks[index-1].blockType))
		a_backFace(posU, currText.back);

	if(pos.y!=(chunkSize-1) && _chunkBlocks[index+chunkSize].transparent() && (bCurr != _chunkBlocks[index+chunkSize].blockType))
		a_upFace(posU, currText.up);

	if(pos.y!=0 && _chunkBlocks[index-chunkSize].transparent() && (bCurr != _chunkBlocks[index-chunkSize].blockType))
		a_downFace(posU, currText.down);

	if(pos.x!=(chunkSize-1) && _chunkBlocks[index+chunkSize*chunkSize].transparent() && (bCurr != _chunkBlocks[index+chunkSize*chunkSize].blockType))
		a_rightFace(posU, currText.right);

	if(pos.x!=0 && _chunkBlocks[index-chunkSize*chunkSize].transparent() && (bCurr != _chunkBlocks[index-chunkSize*chunkSize].blockType))
		a_leftFace(posU, currText.left);
}

void WorldChunk::apply_model()
{
	assert(_wGen!=nullptr);
	
	if(_modelCreated)
		_wGen->_init->set_model(_modelID, _chunkModel);
}

void WorldChunk::remove_model()
{
	assert(_wGen!=nullptr);
	
	_wGen->_init->remove_model(_modelID);
		
	_empty = true;
}

void WorldChunk::update_wall(Direction wall, WorldChunk& checkChunk)
{
	if(_empty)
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
						if(checkChunk.empty() || (checkChunk._chunkBlocks[blockIndex-startingIndex].transparent()
						&& (_chunkBlocks[blockIndex].blockType != checkChunk._chunkBlocks[blockIndex-startingIndex].blockType)))
							a_rightFace(Vec3dCVT<float>(chunkSize-1, y, z)*blockModelSize, _chunkBlocks[blockIndex].texture().right);
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
						if(checkChunk.empty() || (checkChunk._chunkBlocks[blockIndex].transparent()
						&& (_chunkBlocks[blockIndex-startingIndex].blockType != checkChunk._chunkBlocks[blockIndex].blockType)))
							a_leftFace(Vec3dCVT<float>(0, y, z)*blockModelSize, _chunkBlocks[blockIndex-startingIndex].texture().left);
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
					WorldBlock* blockCheck = &checkChunk.block({x, 0, z});
					
					if(blockCurr->blockType!=Block::air)
					{
						if(checkChunk.empty() || (blockCheck->transparent()
						&& (blockCurr->blockType != blockCheck->blockType)))
							a_upFace(Vec3dCVT<float>(x, chunkSize-1, z)*blockModelSize, blockCurr->texture().up);
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
					WorldBlock* blockCheck = &checkChunk.block({x, chunkSize-1, z});
				
					if(blockCurr->blockType!=Block::air)
					{
						if(checkChunk.empty() || (blockCheck->transparent()
						&& (blockCurr->blockType != blockCheck->blockType)))
							a_downFace(Vec3dCVT<float>(x, 0, z)*blockModelSize, blockCurr->texture().down);
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
						if(checkChunk.empty() || (checkChunk._chunkBlocks[blockIndex].transparent()
						&& (_chunkBlocks[blockIndex+chunkSize-1].blockType != checkChunk._chunkBlocks[blockIndex].blockType)))
							a_forwardFace(Vec3dCVT<float>(x, y, chunkSize-1)*blockModelSize, _chunkBlocks[blockIndex+chunkSize-1].texture().forward);
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
						if(checkChunk.empty() || (checkChunk._chunkBlocks[blockIndex+chunkSize-1].transparent()
						&& (_chunkBlocks[blockIndex].blockType != checkChunk._chunkBlocks[blockIndex+chunkSize-1].blockType)))
							a_backFace(Vec3dCVT<float>(x, y, 0)*blockModelSize, _chunkBlocks[blockIndex].texture().back);
					}
				}
			}
			break;
		}
	}
	
	apply_model();
}


void WorldChunk::a_forwardFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
	//i cant write any better code for these, it literally HAS to be hardcoded :/
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_backFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});

	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_leftFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+1, _indexOffset+2, _indexOffset+1, _indexOffset+3, _indexOffset+2});
	
	_indexOffset += 4;
}
void WorldChunk::a_rightFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x+blockModelSize, posU.y, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});	
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_upFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
	_chunkModel.vertices.insert(_chunkModel.vertices.end(),
	{posU.x, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x, _textureOffset*texturePos.y,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y,
	posU.x, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x, _textureOffset*texturePos.y+_textureOffset,
	posU.x+blockModelSize, posU.y+blockModelSize, posU.z+blockModelSize, _textureOffset*texturePos.x+_textureOffset, _textureOffset*texturePos.y+_textureOffset});
	
	_chunkModel.indices.insert(_chunkModel.indices.end(), {_indexOffset, _indexOffset+2, _indexOffset+1, _indexOffset+1, _indexOffset+2, _indexOffset+3});
	
	_indexOffset += 4;
}
void WorldChunk::a_downFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept
{
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
	return {static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z)};
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

int WorldChunk::plants_amount()
{
	return _plantsAmount;
}

void WorldChunk::set_plants_amount(int amount)
{
	_plantsAmount = amount;
}

Vec3d<int> &WorldChunk::position()
{
	return _position;
}

WorldChunk::ChunkBlocks& WorldChunk::blocks()
{
	return _chunkBlocks;
}
