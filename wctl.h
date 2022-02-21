#ifndef WCTL_H
#define WCTL_H

#include <map>
#include <set>
#include <vector>

#include <glcyan.h>
#include <ythreads.h>

#include "world.h"
#include "character.h"

class WorldController
{
public:
	struct MeshInfo
	{
		YandereObject mesh;
		
		bool visible = false;
	};


	WorldController() {};
	WorldController(YandereInitializer* init, Character* mainCharacter, YandereCamera* mainCamera);
	
	void create_world(unsigned blockTexturesID, unsigned seed);
	
	
	void chunk_update(WorldChunk& chunk);
	void chunk_update_full(WorldChunk& chunk, Vec3d<int> blockPos);
	void chunk_update_full(WorldChunk& chunk, WorldTypes::UpdateChunk walls);
	
	
	void wait_threads();
	
	void set_visibles();
	void range_remove();
	
	void add_chunks();
	
	void full_update();
	
	void draw_update();
	
	void update_queued(std::map<Vec3d<int>, WorldTypes::UpdateChunk>& updateChunks);
	void queue_updater(const std::map<Vec3d<int>, WorldTypes::UpdateChunk>& queuedChunks);
	
	
	int render_dist();
	int chunk_radius();
	
	std::map<Vec3d<int>, WorldChunk> worldChunks;

private:
	void chunk_loader(Vec3d<int> chunkPos);
	void update_walls(std::map<Vec3d<int>, WorldTypes::UpdateChunk>::iterator iter);

	float chunk_outside(const Vec3d<int> pos) const;

	void init_chunks();
	
	YandereInitializer* _initer = nullptr;
	Character* _mainCharacter = nullptr;
	YandereCamera* _mainCamera = nullptr;
	std::unique_ptr<WorldGenerator> _worldGen;
	
	std::queue<Vec3d<int>> _initializeChunks;
	
	std::vector<WorldChunk> _chunksLoad;
	
	std::map<Vec3d<int>, MeshInfo> _chunksInfo;
	std::set<Vec3d<int>> _beenLoadedChunks;
	
	std::map<Vec3d<int>, WorldTypes::UpdateChunk> _queuedBlocks;
	std::map<Vec3d<int>, WorldTypes::UpdateChunk> _chunkUpdateWalls;
	
	int _chunkRadius = 6;
	int _renderDist = 5;
	
	bool _worldCreated = false;
	bool _empty = true;
	
	typedef YanderePool<void (WorldController::*)(Vec3d<int>), WorldController*, Vec3d<int>> poolType;
	
	inline static std::mutex _loadChunksMtx;
	
	std::unique_ptr<poolType> _chunkGenPool;
};

#endif
