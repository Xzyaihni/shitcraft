#ifndef WGEN_H
#define WGEN_H

#include <mutex>
#include <map>
#include <list>

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
protected:
	struct VecPos
	{
		Vec3d<int> chunkPos;
		Vec3d<int> originalPos;
		Vec3d<int> blockPos;
		WorldBlock block;
	};

public:
	typedef std::array<WorldTypes::ClimatePoint, chunkSize*chunkSize> ClimateNoise;

	WorldGenerator();
	WorldGenerator(YandereInitializer* init, std::string atlasName);
	
	void seed(unsigned seed);
	
	std::array<float, chunkSize*chunkSize> generate_noise(Vec3d<int> pos, float noiseScale, float noiseStrength);
	ClimateNoise generate_climate(Vec3d<int> pos, float temperatureScale, float humidityScale);
	
	void chunk_gen(WorldChunk& chunk);
	WorldTypes::Biome get_biome(float temperature, float humidity);
	void gen_plants(WorldChunk& genChunk, ClimateNoise& climateArr);
	
	Vec3d<int> get_ground(WorldChunk& checkChunk, int x, int z);
	
	void place_in_chunk(Vec3d<int> originalPos, Vec3d<int> chunkPos, Vec3d<int> blockPos, WorldBlock block);
	void place_in_chunk(std::list<VecPos>& blocks);
	
	std::string atlasName;

protected:
	std::mutex _mtxBlockPlace;
	
	std::list<VecPos> _blockPlaceList;

	YandereInitializer* _init;
	BlockTexAtlas _texAtlas;

	NoiseGenerator _noiseGen;

	unsigned _seed = 1;

	friend class WorldChunk;
	friend class WorldController;
};

#endif
