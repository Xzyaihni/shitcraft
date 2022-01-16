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
		_chunkBlocks = std::vector<WorldBlock>(chunkSize*chunkSize*chunkSize, WorldBlock{Block::stone});
		
		update_states();
		update_mesh();
		
		return;
	}
	
	std::vector<float> noiseMap = _wGen->generate(_position);
	
	_chunkBlocks.clear();
	_chunkBlocks.reserve(chunkSize*chunkSize*chunkSize);
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z)
			{
				float currNoise = noiseMap[x*chunkSize+z]*chunkSize;
			
				if(_position.y*chunkSize+y<currNoise)
				{
					bool currGrass = (_position.y*chunkSize+y+1)>=currNoise;
					_chunkBlocks.emplace_back(WorldBlock{Block::dirt, BlockInfo{currGrass}});
					continue;
				}
				
				_chunkBlocks.emplace_back(WorldBlock{Block::air});
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
		
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z)
			{	
				if(getBlock({x,y,z}).blockType!=Block::air)
				{
					if(z!=(chunkSize-1) && getBlock({x,y,z+1}).isTransparent())
					{
						a_forwardFace({x, y, z});
					}
						
					if(z!=0 && getBlock({x,y,z-1}).isTransparent())
					{
						a_backFace({x, y, z});
					}
					
					if(y!=(chunkSize-1) && getBlock({x,y+1,z}).isTransparent())
					{
						a_upFace({x, y, z});
					}
					
					if(y!=0 && getBlock({x,y-1,z}).isTransparent())
					{
						a_downFace({x, y, z});
					}
					
					if(x!=(chunkSize-1) && getBlock({x+1,y,z}).isTransparent())
					{
						a_rightFace({x, y, z});
					}
					
					if(x!=0 && getBlock({x-1,y,z}).isTransparent())
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
	_wGen->_init->_modelMap[WorldChunk::getModelName(_position)] = _chunkModel;
}

void WorldChunk::update_wall(Direction wall, WorldChunk* checkChunk)
{
	if(_empty || checkChunk->empty())
		return;
		

	switch(wall)
	{
		case Direction::right:
		{
			for(int y = 0; y < chunkSize; ++y)
			{
				for(int z = 0; z < chunkSize; ++z)
				{
					if(!getBlock({chunkSize-1, y, z}).isTransparent() && checkChunk->getBlock({0, y, z}).isTransparent())
					{
						a_rightFace({chunkSize-1, y, z});
					}
				}
			}
			break;
		}
		
		case Direction::left:
		{
			for(int y = 0; y < chunkSize; ++y)
			{
				for(int z = 0; z < chunkSize; ++z)
				{
					if(!getBlock({0, y, z}).isTransparent() && checkChunk->getBlock({chunkSize-1, y, z}).isTransparent())
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
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int y = 0; y < chunkSize; ++y)
				{
					if(!getBlock({x, y, chunkSize-1}).isTransparent() && checkChunk->getBlock({x, y, 0}).isTransparent())
					{
						a_forwardFace({x, y, chunkSize-1});
					}
				}
			}
			break;
		}
		
		case Direction::back:
		{
			for(int x = 0; x < chunkSize; ++x)
			{
				for(int y = 0; y < chunkSize; ++y)
				{
					if(!getBlock({x, y, 0}).isTransparent() && checkChunk->getBlock({x, y, chunkSize-1}).isTransparent())
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
		
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize);
			
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
		
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

	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z);
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
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
	
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x);
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v%2));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
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
				
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize);
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize*(v>1));
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v%2));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
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

	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y+blockModelSize);
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v>1));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
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
	
	for(int v = 0; v < 4; ++v)
	{
		_chunkModel.vertices.push_back(blockModelSize*pos.x+blockModelSize*(v%2));
		_chunkModel.vertices.push_back(blockModelSize*pos.y);
		_chunkModel.vertices.push_back(blockModelSize*pos.z+blockModelSize*(v>1));
		
		_chunkModel.vertices.push_back(textureOffset*(texturePos.x+v%2));
		_chunkModel.vertices.push_back(textureOffset*(texturePos.y+(v>1)));
	}
	
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
