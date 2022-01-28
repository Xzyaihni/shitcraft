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
	struct UpdateChunk
	{
		bool right = true;
		bool left = true;
		bool up = true;
		bool down = true;
		bool forward = true;
		bool back = true;
		
		bool walls_or() {return right || left || up || down || forward || back;};
	};

	struct MeshInfo
	{
		YandereObject mesh;
		
		bool visible = false;
	};
	
	struct PosWalls
	{
		Vec3d<int> chunkPos;
		UpdateChunk walls;
	};


	WorldController() {};
	WorldController(YandereInitializer* init, Character* mainCharacter, YandereCamera* mainCamera);
	
	void create_world(unsigned seed);
	
	
	void wait_threads();
	
	void set_visibles();
	void range_remove();
	
	void full_update();
	
	void draw_update();
	
	std::map<Vec3d<int>, UpdateChunk> update_queued();
	void queue_updater(const std::map<Vec3d<int>, UpdateChunk>& queuedChunks);
	
	
	int render_dist();
	int chunk_radius();
	
	std::map<Vec3d<int>, WorldChunk> loadedChunks;

private:
	void chunk_loader(Vec3d<int> chunkPos);
	void update_walls(PosWalls currChunk);

	float chunk_outside(const Vec3d<int> pos) const;

	void init_chunks();
	
	YandereInitializer* _initer = nullptr;
	Character* _mainCharacter = nullptr;
	YandereCamera* _mainCamera = nullptr;
	std::unique_ptr<WorldGenerator> _worldGen;
	
	std::queue<Vec3d<int>> _initializeChunks;
	
	std::map<Vec3d<int>, MeshInfo> _chunksInfo;
	std::set<Vec3d<int>> _beenLoadedChunks;
	
	std::map<Vec3d<int>, UpdateChunk> _chunkUpdateWalls;
	
	int _chunkRadius = 6;
	int _renderDist = 5;
	
	bool _worldCreated = false;
	bool _empty = true;
};

namespace wctl_mutex
{
	static std::mutex loadedChunks;
	static std::mutex wallUpdater;
	static std::mutex initQueue;
	
	static YanderePool<void (WorldController::*)(Vec3d<int>), WorldController*, Vec3d<int>> cGenPool;
	static YanderePool<void (WorldController::*)(WorldController::PosWalls), WorldController*, WorldController::PosWalls> cWallPool;
};

#endif
