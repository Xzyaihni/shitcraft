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
	typedef std::array<WorldBlock, chunkSize*chunkSize*chunkSize> ChunkBlocks;

	WorldChunk() {};
	WorldChunk(WorldGenerator* wGen, Vec3d<int> pos);
	
	void update_mesh();
	void update_wall(Direction wall, WorldChunk& checkChunk);
	
	void apply_model();
	void remove_model();
	
	void update_states();
	
	void shared_place(Vec3d<int> position, WorldBlock block);
	static Vec3d<int> active_chunk(Vec3d<int> pos);
	static Vec3d<int> active_chunk(Vec3d<float> pos);
	
	Vec3d<int> closest_block(Vec3d<float> pos);
	WorldBlock& block(Vec3d<int> pos);
	
	void update_block_walls(const Vec3d<int> pos) const noexcept;
	void update_block_walls(const Vec3d<int> pos, const int index) const noexcept;
	
	void set_empty(bool state);
	bool empty();
	bool has_transparent();
	bool check_empty();
	
	int plants_amount();
	void set_plants_amount(int amount);
	
	Vec3d<int>& position();
	
	ChunkBlocks& blocks();
	
	static std::string model_name(const Vec3d<int> pos);

private:
	void a_forwardFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;
	void a_backFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;
	void a_leftFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;
	void a_rightFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;
	void a_upFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;
	void a_downFace(const Vec3d<float> posU, const WorldTypes::TexPos texturePos) const noexcept;

	WorldGenerator* _wGen = nullptr;
	
	ChunkBlocks _chunkBlocks;
	
	mutable YandereModel _chunkModel;
	std::string _modelName;
	
	mutable unsigned _indexOffset;
	
	Vec3d<int> _position;
	
	bool _empty = true;
	
	int _plantsAmount = 0;
	
	int _textureWidth;
	int _textureHeight;
	int _textureHBlocks;
	int _textureVBlocks;
	float _textureOffset;
};

#endif
