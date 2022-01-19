#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>

#include "world.h"
#include "inventory.h"
#include "types.h"

#include <yanconv.h>

using namespace WorldTypes;

BlockTexAtlas::BlockTexAtlas()
{
}

BlockTexAtlas::BlockTexAtlas(int width, int height) : _width(width), _height(height)
{
}

void BlockTexAtlas::setHorizontalBlocks(int hBlocks)
{
	_horizontalBlocks = hBlocks;
	_texOffset = (1.0f/_width*(_width/hBlocks));
	setGlobals();
}

void BlockTexAtlas::setVerticalBlocks(int vBlocks)
{
	_verticalBlocks = vBlocks;
	_texOffset = (1.0f/_height*(_height/vBlocks));
	setGlobals();
}

void BlockTexAtlas::setGlobals()
{
	textureWidth = _width;
	textureHeight = _height;
	textureHBlocks = _horizontalBlocks;
	textureVBlocks = _verticalBlocks;
	textureOffset = _texOffset;
}


WorldGenerator::WorldGenerator()
{
}

WorldGenerator::WorldGenerator(YandereInitializer* init, std::string atlasName) : _init(init),
atlasName(atlasName), _texAtlas(init->_textureMap[atlasName].width(), init->_textureMap[atlasName].height())
{
	_texAtlas.setHorizontalBlocks(8);
	_texAtlas.setVerticalBlocks(8);

	genHeight = 4;
}

void WorldGenerator::changeSeed(unsigned seed)
{
	_noiseGen = NoiseGenerator(seed);
}

std::vector<float> WorldGenerator::generate(Vec3d<int> pos)
{
	float addTerrainSmall = terrainSmallScale/static_cast<float>(chunkSize);
	float addTerrainMid = terrainMidScale/static_cast<float>(chunkSize);
	float addTerrainLarge = terrainLargeScale/static_cast<float>(chunkSize);

	std::vector<float> noiseMap;
	noiseMap.reserve(chunkSize*chunkSize);
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z)
		{
			float smallNoise = _noiseGen.noise(pos.x*terrainSmallScale+x*addTerrainSmall, pos.z*terrainSmallScale+z*addTerrainSmall)/4;
			float midNoise = _noiseGen.noise(pos.x*terrainMidScale+x*addTerrainMid, pos.z*terrainMidScale+z*addTerrainMid);
			float largeNoise = _noiseGen.noise(pos.x*terrainLargeScale+x*addTerrainLarge, pos.z*terrainLargeScale+z*addTerrainLarge)*2;
		
			noiseMap.emplace_back(midNoise*largeNoise+smallNoise);
		}
	}
	
	return std::move(noiseMap);
}

std::vector<float> WorldGenerator::generate_biomes(Vec3d<int> pos)
{
}


void WorldBlock::updateBlock()
{
}

Loot WorldBlock::breakBlock()
{
	return Loot{};
}

TextureFace WorldBlock::getTexture()
{
	switch(blockType)
	{
		case Block::dirt:
			return info.grassy ? TextureFace{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 1}, {0, 2}}
			: TextureFace{{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}};
			
		case Block::stone:
			return TextureFace{{1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}};
		
		default:
			return TextureFace{};
	}
}

bool WorldBlock::isTransparent()
{
	switch(blockType)
	{
		case Block::air:
			return true;
		
		default:
			return false;
	}
}


WorldChunk::WorldChunk()
{
}

WorldChunk::WorldChunk(WorldGenerator* wGen, Vec3d<int> pos) : _wGen(wGen), _position(pos)
{
	_modelName = WorldChunk::getModelName(_position);
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
				if(_chunkBlocks[blockIndex].isTransparent())
				{
					return true;
				}
			}
		}
	}
	return false;
}

void WorldChunk::chunk_gen()
{	
	if(_position.y>_wGen->genHeight)
	{
		_empty = true;
		return;
	} else
	{
		_empty = false;
	}
	
	bool overground = _position.y>=0;
	
	if(!overground)
	{
		_chunkBlocks.fill(WorldBlock{Block::stone});
		
		update_states();
		
		_chunkModel = YandereModel();
		apply_model();
		
		return;
	}
	
	std::vector<float> noiseMap = _wGen->generate(_position);
	
	
	int blockIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{
				float currNoise = noiseMap[x*chunkSize+z]*chunkSize;
			
				if(_position.y*chunkSize+y<currNoise)
				{
					bool currGrass = (_position.y*chunkSize+y+1)>=currNoise;
					_chunkBlocks[blockIndex] = WorldBlock{Block::dirt, BlockInfo{currGrass}};
					continue;
				}
				
				_chunkBlocks[blockIndex] = WorldBlock{Block::air};
			}
		}
	}
	
	update_states();
	
	update_mesh();
}

void WorldChunk::update_states()
{
	if(_empty)
		return;

	std::for_each(std::execution::par_unseq, _chunkBlocks.begin(), _chunkBlocks.end(), [](WorldBlock& block)
	{
			block.updateBlock();
	});
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
					if(z!=(chunkSize-1) && _chunkBlocks[blockIndex+1].isTransparent())
					{
						a_forwardFace({x, y, z});
					}
						
					if(z!=0 && _chunkBlocks[blockIndex-1].isTransparent())
					{
						a_backFace({x, y, z});
					}
					
					if(y!=(chunkSize-1) && _chunkBlocks[blockIndex+chunkSize].isTransparent())
					{
						a_upFace({x, y, z});
					}
					
					if(y!=0 && _chunkBlocks[blockIndex-chunkSize].isTransparent())
					{
						a_downFace({x, y, z});
					}
					
					if(x!=(chunkSize-1) && _chunkBlocks[blockIndex+chunkSize*chunkSize].isTransparent())
					{
						a_rightFace({x, y, z});
					}
					
					if(x!=0 && _chunkBlocks[blockIndex-chunkSize*chunkSize].isTransparent())
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
	_wGen->_init->_modelMap[_modelName] = _chunkModel;
}

void WorldChunk::remove_model()
{
	if(_wGen->_init->_modelMap.count(_modelName)!=0)
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
					if(!_chunkBlocks[blockIndex].isTransparent() && checkChunk->_chunkBlocks[blockIndex-startingIndex].isTransparent())
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
					if(!_chunkBlocks[blockIndex-startingIndex].isTransparent() && checkChunk->_chunkBlocks[blockIndex].isTransparent())
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
					if(!getBlock({x, chunkSize-1, z}).isTransparent() && checkChunk->getBlock({x, 0, z}).isTransparent())
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
					if(!getBlock({x, 0, z}).isTransparent() && checkChunk->getBlock({x, chunkSize-1, z}).isTransparent())
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
					if(!_chunkBlocks[blockIndex+chunkSize-1].isTransparent() && checkChunk->_chunkBlocks[blockIndex].isTransparent())
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
					if(!_chunkBlocks[blockIndex].isTransparent() && checkChunk->_chunkBlocks[blockIndex+chunkSize-1].isTransparent())
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
	TexPos texturePos = getBlock(pos).getTexture().forward;
	
	_chunkModel.vertices.reserve(20);
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize);
			
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);
		_chunkModel.indices.push_back(_indexOffset+2);
	}
	
	_indexOffset += 4;
}
void WorldChunk::a_backFace(Vec3d<int> pos)
{
	TexPos texturePos = getBlock(pos).getTexture().back;

	_chunkModel.vertices.reserve(20);
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z);
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);							
		_chunkModel.indices.push_back(_indexOffset+2);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);
	}
	
	_indexOffset += 4;
}
void WorldChunk::a_leftFace(Vec3d<int> pos)
{
	TexPos texturePos = getBlock(pos).getTexture().left;
	
	_chunkModel.vertices.reserve(20);
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x);
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v%2));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);					
		_chunkModel.indices.push_back(_indexOffset+2);
	}
	
	_indexOffset += 4;
}
void WorldChunk::a_rightFace(Vec3d<int> pos)
{
	TexPos texturePos = getBlock(pos).getTexture().right;
		
	_chunkModel.vertices.reserve(20);	
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize);
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v%2));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);
		_chunkModel.indices.push_back(_indexOffset+2);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);
	}
	
	_indexOffset += 4;
}
void WorldChunk::a_upFace(Vec3d<int> pos)
{
	TexPos texturePos = getBlock(pos).getTexture().up;

	_chunkModel.vertices.reserve(20);
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize);
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v>1));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);
		_chunkModel.indices.push_back(_indexOffset+2);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);
	}
	
	_indexOffset += 4;
}
void WorldChunk::a_downFace(Vec3d<int> pos)
{
	TexPos texturePos = getBlock(pos).getTexture().down;
	
	_chunkModel.vertices.reserve(20);
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y);
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v>1));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
	_chunkModel.indices.reserve(6);
	for(int vi = 0; vi < 2; ++vi)
	{
		_chunkModel.indices.push_back(_indexOffset+vi);
		_chunkModel.indices.push_back(_indexOffset+1+vi*2);							
		_chunkModel.indices.push_back(_indexOffset+2);
	}
	
	_indexOffset += 4;
}

Vec3d<int> WorldChunk::closestBlock(Vec3d<float> pos)
{
	return {static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)), static_cast<int>(std::round(pos.z))};
}

WorldBlock& WorldChunk::getBlock(Vec3d<int> pos)
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

std::string WorldChunk::getModelName(Vec3d<int> pos)
{
	std::string chunkModelName = "!cHUNK" + std::to_string(pos.x);
	chunkModelName += " ";
	chunkModelName += std::to_string(pos.y);
	chunkModelName += " ";
	chunkModelName += std::to_string(pos.z);
	
	return chunkModelName;
}
