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
}

void WorldGenerator::seed(unsigned seed)
{
	_seed = seed;
	_noiseGen = NoiseGenerator(seed);
}

std::array<float, chunkSize*chunkSize> WorldGenerator::generate_noise(Vec3d<int> pos, float noiseScale, float noiseStrength)
{
	float addNoise = noiseScale/static_cast<float>(chunkSize);

	std::array<float, chunkSize*chunkSize> noiseArr;
	
	int noiseIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int z = 0; z < chunkSize; ++z, ++noiseIndex)
		{
			noiseArr[noiseIndex] = _noiseGen.noise(pos.x*noiseScale+x*addNoise, pos.z*noiseScale+z*addNoise)*noiseStrength;
		}
	}
	
	return std::move(noiseArr);
}

std::array<ClimatePoint, chunkSize*chunkSize> WorldGenerator::generate_climate(Vec3d<int> pos, float temperatureScale, float humidityScale)
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

void WorldGenerator::chunk_gen(WorldChunk& chunk)
{
	const float genHeight = 2.25f;
	const float genDepth = 0;

	if(chunk.position().y>genHeight)
	{
		chunk.set_empty(true);
		return;
	}
	
	chunk.set_empty(false);
	
	bool overground = chunk.position().y>=genDepth;
	
	WorldChunk::ChunkBlocks& currBlocks = chunk.blocks();
	
	if(!overground)
	{
		currBlocks.fill(WorldBlock{Block::stone});
		
		chunk.update_states();
		
		return;
	}
	
	std::array<float, chunkSize*chunkSize> smallNoiseArr = generate_noise(chunk.position(), 1.05f, 0.25f);
	std::array<float, chunkSize*chunkSize> mediumNoiseArr = generate_noise(chunk.position(), 0.22f, 1);
	std::array<float, chunkSize*chunkSize> largeNoiseArr = generate_noise(chunk.position(), 0.005f, 2);
	
	std::array<ClimatePoint, chunkSize*chunkSize> climateArr = generate_climate(chunk.position(), 0.0136f, 0.0073f);
	
	
	int blockIndex = 0;
	
	for(int x = 0; x < chunkSize; ++x)
	{
		for(int y = 0; y < chunkSize; ++y)
		{
			for(int z = 0; z < chunkSize; ++z, ++blockIndex)
			{
				int mapsIndex = x*chunkSize+z;
				float currNoise;
			
				Biome currBiome = get_biome(climateArr[mapsIndex].temperature, climateArr[mapsIndex].humidity);
			
				switch(currBiome)
				{
					case Biome::hell:
					{
						currNoise = (largeNoiseArr[mapsIndex]/4*mediumNoiseArr[mapsIndex]/4+smallNoiseArr[mapsIndex]/4) * chunkSize;
						break;
					}
				
					default:
					{
						currNoise = (largeNoiseArr[mapsIndex]*mediumNoiseArr[mapsIndex]+smallNoiseArr[mapsIndex]) * chunkSize;
						break;
					}
				}
			
				if(chunk.position().y*chunkSize+y<currNoise)
				{
					switch(currBiome)
					{
						case Biome::desert:
						{
							currBlocks[blockIndex] = WorldBlock{Block::sand};
							break;
						}
						
						case Biome::hell:
						{
							currBlocks[blockIndex] = WorldBlock{Block::lava};
							break;
						}
						
						default:
						case Biome::forest:
						{
							bool currGrass = (chunk.position().y*chunkSize+y+1)>=currNoise;
							currBlocks[blockIndex] = WorldBlock{Block::dirt, BlockInfo{currGrass}};
							break;
						}
					}
				} else
				{
					currBlocks[blockIndex] = WorldBlock{Block::air};
				}
			}
		}
	}
	
	gen_plants(chunk, climateArr);
	
	chunk.update_states();
	chunk.update_mesh();
}

Biome WorldGenerator::get_biome(float temperature, float humidity)
{
	if(temperature>0.5f && humidity<0.5f)
	{
		if(temperature>0.65f)
			return Biome::hell;
			
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
	std::mt19937 sGen(_seed^(genChunk.position().x)^(genChunk.position().z));
	std::uniform_int_distribution distrib(1, 1000);
	
	std::uniform_int_distribution plantDistrib(1, 8);

	int plantsGenerated = 0;

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
						++plantsGenerated;
					
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
			
				case Biome::forest:
				{
					++plantsGenerated;
				
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
				
				default:
					break;
			}
		}
	}
	
	genChunk.set_plants_amount(plantsGenerated);
}


void WorldGenerator::place_in_chunk(Vec3d<int> originalPos, Vec3d<int> chunkPos, Vec3d<int> blockPos, WorldBlock block)
{
	std::lock_guard<std::mutex> lockB(_mtxBlockPlace);

	_blockPlaceList.emplace_back(chunkPos, originalPos, blockPos, block);
}

void WorldGenerator::place_in_chunk(std::list<VecPos>& blocks)
{
	std::lock_guard<std::mutex> lockB(_mtxBlockPlace);

	_blockPlaceList.splice(_blockPlaceList.end(), blocks);
}
