#include <iostream>
#include <algorithm>
#include <chrono>

#include "world.h"
#include "noise.h"
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
}

std::vector<float> WorldGenerator::generate(Vec3d<int> pos)
{
	NoiseGenerator noiseGen{seed};

	float noiseAdd = noiseParts/static_cast<float>(chunkSize);

	std::vector<float> noiseMap;
	noiseMap.reserve(chunkSize);
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z)
		{
			//noiseMap.emplace_back(noiseGen.noise(pos.x+0.5f, pos.z+0.5f));
			noiseMap.emplace_back(noiseGen.noise(pos.x*noiseParts+x*noiseAdd, pos.z*noiseParts+z*noiseAdd));
		}
	}
	
	return std::move(noiseMap);
}


void WorldBlock::updateBlock()
{
}

Loot WorldBlock::breakBlock()
{
	return Loot();
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
	if(_position.y*chunkSize>_wGen->genHeight)
	{
		_empty = true;
		return;
	} else
	{
		_empty = false;
	}
	
	bool overground = _position.y>=0;
	
	_chunkBlocks.clear();
	_chunkBlocks.reserve(chunkSize*chunkSize*chunkSize);
	
	std::vector<float> noiseMap;
	if(overground)
		noiseMap = _wGen->generate(_position);
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z)
			{
				float currNoise;
				if(overground)
				{
					currNoise = noiseMap[x*chunkSize+z]*_wGen->genHeight;
				} else
				{
					currNoise = 1;
				}
			
				if(_position.y*chunkSize+y<currNoise)
				{
					bool currGrass = (_position.y*chunkSize+y+1)>=currNoise;
					_chunkBlocks.emplace_back(WorldBlock{Block::dirt, BlockInfo{currGrass}});
				} else
				{
					_chunkBlocks.emplace_back(WorldBlock{Block::air});
				}
			}
		}
	}
	
	updateStates();
	
	update_mesh();
}

void WorldChunk::updateStates()
{
	if(_empty)
		return;

	std::for_each(_chunkBlocks.begin(), _chunkBlocks.end(), [](WorldBlock& block)
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
				}
			}
		}
	}
		
	_wGen->_init->_modelMap[WorldChunk::getModelName(_position)] = _chunkModel;
}

void update_wall(Direction wall, WorldChunk* checkChunk = nullptr)
{
	//doing rn
	return;
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
	return _empty;
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
