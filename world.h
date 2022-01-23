#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <memory>
#include <glcyan.h>

#include "wgen.h"
#include "wblock.h"


class WorldChunk
{
public:
	WorldChunk() {};
	WorldChunk(WorldGenerator* wGen, Vec3d<int> pos);
	
	void chunk_gen();
	void update_mesh();
	void update_wall(Direction wall, WorldChunk* checkChunk);
	
	void apply_model();
	void remove_model();
	
	void update_states();
	
	void shared_place(Vec3d<int> position, WorldBlock block, bool replace = false);
	static Vec3d<int> active_chunk(Vec3d<int> pos);
	static Vec3d<int> active_chunk(Vec3d<float> pos);
	
	Vec3d<int> closest_block(Vec3d<float> pos);
	WorldBlock& block(Vec3d<int> pos);
	
	void update_block_walls(Vec3d<int> pos);
	void update_block_walls(Vec3d<int> pos, int index);
	
	void set_empty(bool state);
	bool empty();
	bool has_transparent();
	bool check_empty();
	
	Vec3d<int> position();
	
	static std::string model_name(Vec3d<int> pos);

private:
	void r_forwardFace(Vec3d<int> pos);
	void r_backFace(Vec3d<int> pos);
	void r_leftFace(Vec3d<int> pos);
	void r_rightFace(Vec3d<int> pos);
	void r_upFace(Vec3d<int> pos);
	void r_downFace(Vec3d<int> pos);

	void a_forwardFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);
	void a_backFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);
	void a_leftFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);
	void a_rightFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);
	void a_upFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);
	void a_downFace(Vec3d<int> pos, WorldTypes::TexPos texturePos);

	WorldGenerator* _wGen = nullptr;
	
	std::array<WorldBlock, chunkSize*chunkSize*chunkSize> _chunkBlocks;
	
	YandereModel _chunkModel = YandereModel();
	std::string _modelName;
	
	unsigned _indexOffset;
	
	Vec3d<int> _position;
	
	bool _empty = true;
	
	int _textureWidth;
	int _textureHeight;
	int _textureHBlocks;
	int _textureVBlocks;
	float _textureOffset;
};

#endif
