#include <array>
#include <iostream>
#include <random>

#include "wgen.h"
#include "world.h"

using namespace WorldTypes;


BlockTexAtlas::BlockTexAtlas()
{
}

BlockTexAtlas::BlockTexAtlas(int width, int height) : _width(width), _height(height)
{
}

void BlockTexAtlas::set_horizontal_blocks(int hBlocks)
{
	_horizontalBlocks = hBlocks;
	_texOffset = (1.0f/_width*(_width/hBlocks));
}

void BlockTexAtlas::set_vertical_blocks(int vBlocks)
{
	_verticalBlocks = vBlocks;
	_texOffset = (1.0f/_height*(_height/vBlocks));
}


WorldGenerator::WorldGenerator()
{
}

WorldGenerator::WorldGenerator(YandereInitializer* init, std::string atlasName) : _init(init),
atlasName(atlasName), _texAtlas(init->_textureMap[atlasName].width(), init->_textureMap[atlasName].height())
{
	_texAtlas.set_horizontal_blocks(8);
	_texAtlas.set_vertical_blocks(8);

	genHeight = 4;
}

void WorldGenerator::seed(unsigned seed)
{
	_seed = seed;
	_noiseGen = NoiseGenerator(seed);
}

std::array<float, chunkSize*chunkSize> WorldGenerator::generate(Vec3d<int> pos)
{
	float addTerrainSmall = terrainSmallScale/static_cast<float>(chunkSize);
	float addTerrainMid = terrainMidScale/static_cast<float>(chunkSize);
	float addTerrainLarge = terrainLargeScale/static_cast<float>(chunkSize);

	std::array<float, chunkSize*chunkSize> noiseArr;
	
	int noiseIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z, ++noiseIndex)
		{
			float smallNoise = _noiseGen.noise(pos.x*terrainSmallScale+x*addTerrainSmall, pos.z*terrainSmallScale+z*addTerrainSmall)/4;
			float midNoise = _noiseGen.noise(pos.x*terrainMidScale+x*addTerrainMid, pos.z*terrainMidScale+z*addTerrainMid);
			float largeNoise = _noiseGen.noise(pos.x*terrainLargeScale+x*addTerrainLarge, pos.z*terrainLargeScale+z*addTerrainLarge)*2;
		
			noiseArr[noiseIndex] = midNoise*largeNoise+smallNoise;
		}
	}
	
	return std::move(noiseArr);
}

std::array<ClimatePoint, chunkSize*chunkSize> WorldGenerator::generate_climate(Vec3d<int> pos)
{
	float addTemperature = temperatureScale/static_cast<float>(chunkSize);
	float addHumidity = humidityScale/static_cast<float>(chunkSize);

	std::array<ClimatePoint, chunkSize*chunkSize> noiseArr;
	
	int noiseIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z, ++noiseIndex)
		{
			float temperatureNoise = _noiseGen.noise(pos.x*temperatureScale+x*addTemperature, pos.z*temperatureScale+z*addTemperature);
			float humidityNoise = _noiseGen.noise(pos.x*humidityScale+x*addHumidity, pos.z*humidityScale+z*addHumidity);
		
			noiseArr[noiseIndex] = ClimatePoint{temperatureNoise, humidityNoise};
		}
	}
	
	return std::move(noiseArr);
}

Biome WorldGenerator::get_biome(float temperature, float humidity)
{
	if(temperature>0.5f && humidity<0.5f)
	{
		return Biome::desert;
	} else
	{
		return Biome::forest;
	}
}

Vec3d<int> WorldGenerator::get_ground(WorldChunk& checkChunk, int x, int z)
{
	Vec3d<int> groundPos = {0, 0, 0};
					
	for(int i = 0; i < chunkSize; ++i)
	{
		if(checkChunk.block({x, i, z}).transparent())
		{
			groundPos = {x, i, z};
			break;
		}
	}
	
	return groundPos;
}

void WorldGenerator::gen_plants(WorldChunk& genChunk, std::array<ClimatePoint, chunkSize*chunkSize>& climateArr)
{
	std::mt19937 sGen(_seed);
	std::uniform_int_distribution distrib(1, 1000);
	
	std::uniform_int_distribution plantDistrib(1, 8);

	int pointIndex = 0;
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z, ++pointIndex)
		{
			switch(get_biome(climateArr[pointIndex].temperature, climateArr[pointIndex].humidity))
			{
				case Biome::desert:
				{
					if(distrib(sGen) < (climateArr[pointIndex].humidity-0.10f)*10)
					{
						Vec3d<int> groundPos = get_ground(genChunk, x, z);
					
						if(groundPos.y==0)
							continue;
						
						
						int cactusHeight = 2+plantDistrib(sGen);
							
						for(int i = 0; i < cactusHeight; ++i)
						{
							genChunk.shared_place({groundPos.x, groundPos.y+i, groundPos.z}, WorldBlock{Block::cactus});
						}
					}
					break;
				}
			
				default:
				case Biome::forest:
				{
					if(distrib(sGen) < (climateArr[pointIndex].humidity-0.45f)*50)
					{	
						Vec3d<int> groundPos = get_ground(genChunk, x, z);
					
						if(groundPos.y==0)
							continue;
							
						
						int treeHeight = plantDistrib(sGen);
						
						for(int i = 0; i < treeHeight; ++i)
						{
							genChunk.shared_place({groundPos.x, groundPos.y+i, groundPos.z}, WorldBlock{Block::log});
							
							int nearestSquare = (std::clamp(treeHeight-i, 0, 2))*2+1;
							
							int halfSquare = nearestSquare/2;
							
							for(int tx = 0; tx < nearestSquare; ++tx)
							{
								for(int ty = 0; ty < nearestSquare; ++ty)
								{
									if(tx-halfSquare==0 && ty-halfSquare==0)
										continue;
									
									genChunk.shared_place({groundPos.x+tx-halfSquare, groundPos.y+i+1, groundPos.z+ty-halfSquare}, WorldBlock{Block::leaf});
								}
							}
						}
						
						genChunk.shared_place({groundPos.x, groundPos.y+treeHeight, groundPos.z}, WorldBlock{Block::leaf});
					}
					break;
				}
			}
		}
	}
}


void WorldGenerator::place_in_chunk(Vec3d<int> chunkPos, Vec3d<int> blockPos, WorldBlock block, bool replace)
{
	std::unique_lock<std::mutex> lockB(_mtxBlockPlace);

	_blockPlaceQueue.push({chunkPos, blockPos, block, replace});
}
