#ifndef WGEN_H
#define WGEN_H

#include <glcyan.h>

#include "noise.h"
#include "types.h"


namespace WorldTypes
{
	struct TexPos
	{
		int x;
		int y;
	};
	
	struct ClimatePoint
	{
		float temperature;
		float humidity;
	};

	enum Biome
	{
		forest = 0,
		desert
	};
	
	enum Block
	{
		air = 0,
		dirt,
		stone,
		sand,
		log,
		leaf,
		cactus
	};
	
	struct TextureFace
	{
		WorldTypes::TexPos forward;
		WorldTypes::TexPos back;
		WorldTypes::TexPos right;
		WorldTypes::TexPos left;
		WorldTypes::TexPos up;
		WorldTypes::TexPos down;
	};
	
	struct BlockInfo
	{
		bool grassy = false;
	};
	
	const float blockModelSize = 1.0f/static_cast<float>(chunkSize);
};

class Loot;

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
	
	float terrainSmallScale = 2;
	float terrainMidScale = 1;
	float terrainLargeScale = 0.25f;
	float temperatureScale = 0.1f;
	float humidityScale = 0.2f;
	float genHeight = chunkSize;
	
	std::string atlasName;

protected:
	YandereInitializer* _init;
	BlockTexAtlas _texAtlas;

	NoiseGenerator _noiseGen;

	unsigned _seed = 1;

	friend class WorldChunk;
};

#endif
