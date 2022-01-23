#ifndef WGEN_H
#define WGEN_H

#include <mutex>
#include <queue>

#include <glcyan.h>

#include "noise.h"
#include "types.h"
#include "worldtypes.h"
#include "wblock.h"

class BlockTexAtlas
{
public:
	BlockTexAtlas();
	BlockTexAtlas(int width, int height);
	
	void set_horizontal_blocks(int hBlocks);
	void set_vertical_blocks(int vBlocks);
	
protected:
	int _width;
	int _height;
	
	int _horizontalBlocks;
	int _verticalBlocks;
	
	float _texOffset;
	
	friend class WorldChunk;
};


class WorldChunk;

class WorldGenerator
{
public:
	WorldGenerator();
	WorldGenerator(YandereInitializer* init, std::string atlasName);
	
	void seed(unsigned seed);
	
	std::array<float, chunkSize*chunkSize> generate(Vec3d<int> pos);
	std::array<WorldTypes::ClimatePoint, chunkSize*chunkSize> generate_climate(Vec3d<int> pos);
	
	WorldTypes::Biome get_biome(float temperature, float humidity);
	void gen_plants(WorldChunk& genChunk, std::array<WorldTypes::ClimatePoint, chunkSize*chunkSize>& climateArr);
	
	Vec3d<int> get_ground(WorldChunk& checkChunk, int x, int z);
	
	void place_in_chunk(Direction callChunkSide, Vec3d<int> chunkPos, Vec3d<int> blockPos, WorldBlock block, bool replace);
	
	float terrainSmallScale = 2;
	float terrainMidScale = 1;
	float terrainLargeScale = 0.25f;
	float temperatureScale = 0.1f;
	float humidityScale = 0.2f;
	float genHeight = chunkSize;
	
	std::string atlasName;

protected:
	struct BlockChunkPos
	{
		Direction callChunkSide;
		Vec3d<int> chunkPos;
		Vec3d<int> blockPos;
		
		WorldBlock block;
		
		bool replace;
	};
	
	std::mutex _mtxBlockPlace;
	
	std::queue<BlockChunkPos> _blockPlaceQueue;

	YandereInitializer* _init;
	BlockTexAtlas _texAtlas;

	NoiseGenerator _noiseGen;

	unsigned _seed = 1;

	friend class WorldChunk;
	friend class WorldController;
};

#endif
