#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <memory>
#include <glcyan.h>

#include "types.h"
#include "noise.h"

namespace WorldTypes
{
	struct TexPos
	{
		int x;
		int y;
	};
	
	enum Block
	{
		air = 0,
		dirt,
		stone
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
	
	static int textureWidth;
	static int textureHeight;
	
	static int textureHBlocks;
	static int textureVBlocks;
	
	static float textureOffset;
};

class Loot;

class BlockTexAtlas
{
public:
	BlockTexAtlas();
	BlockTexAtlas(int width, int height);
	
	void setHorizontalBlocks(int hBlocks);
	void setVerticalBlocks(int vBlocks);
	
protected:
	int _width;
	int _height;
	
	int _horizontalBlocks;
	int _verticalBlocks;
	
	float _texOffset;
	
	friend class WorldChunk;
	
public:
	void setGlobals();
};

class WorldGenerator
{
public:
	WorldGenerator();
	WorldGenerator(YandereInitializer* init, std::string atlasName);
	
	void changeSeed(unsigned seed);
	
	std::vector<float> generate(Vec3d<int> pos);
	std::vector<float> generate_biomes(Vec3d<int> pos);
	
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

	friend class WorldChunk;
};

struct WorldBlock
{
public:
	int blockType;	
	WorldTypes::BlockInfo info = {}; 

	void updateBlock();
	Loot breakBlock();
	
	WorldTypes::TextureFace getTexture();
	bool isTransparent();
};


class WorldChunk
{
public:
	WorldChunk();
	WorldChunk(WorldGenerator* wGen, Vec3d<int> pos);
	
	void chunk_gen();
	void update_mesh();
	void update_wall(Direction wall, WorldChunk* checkChunk);
	
	void apply_model();
	
	void update_states();
	
	Vec3d<int> closestBlock(Vec3d<float> pos);
	WorldBlock& getBlock(Vec3d<int> pos);
	
	bool empty();
	
	Vec3d<int> position();
	
	static std::string getModelName(Vec3d<int> pos);

private:
	void a_forwardFace(Vec3d<int> pos);
	void a_backFace(Vec3d<int> pos);
	void a_leftFace(Vec3d<int> pos);
	void a_rightFace(Vec3d<int> pos);
	void a_upFace(Vec3d<int> pos);
	void a_downFace(Vec3d<int> pos);

	WorldGenerator* _wGen = nullptr;
	
	std::vector<WorldBlock> _chunkBlocks;
	
	YandereModel _chunkModel;
	int _indexOffset;
	
	Vec3d<int> _position;
	
	bool _empty = true;
};

#endif
