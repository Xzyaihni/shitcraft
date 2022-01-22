#ifndef WCTL_H
#define WCTL_H

#include <map>

#include <glcyan.h>
#include <ythreads.h>

#include "world.h"
#include "character.h"

class WorldController
{
public:
	struct MeshVisible
	{
		YandereObject mesh;
		bool visible = false;
	};
	
	struct UpdateChunk
	{
		Vec3d<int> pos;
		bool right = true;
		bool left = true;
		bool up = true;
		bool down = true;
		bool forward = true;
		bool back = true;
		
		bool walls_or() {return right || left || up || down || forward || back;};
	};


	WorldController() {};
	WorldController(YandereInitializer* init, Character* mainCharacter, YandereCamera* mainCamera);
	
	void create_world(unsigned seed);
	
	
	void wait_threads();
	
	void set_visibles();
	void range_remove();
	
	void full_update();
	
	void draw_update();
	
	void update_queued();
	
	
	int render_dist();
	int chunk_radius();
	
	std::map<Vec3d<int>, WorldChunk> loadedChunks;

private:
	void chunk_loader(Vec3d<int> chunkPos);
	void update_walls(UpdateChunk currChunk);

	void init_chunks();
	
	YandereInitializer* _initer = nullptr;
	Character* _mainCharacter = nullptr;
	YandereCamera* _mainCamera = nullptr;
	std::unique_ptr<WorldGenerator> _worldGen;
	
	std::map<Vec3d<int>, MeshVisible> _drawMeshes;
	
	std::queue<Vec3d<int>> _initializeChunks;
	std::queue<UpdateChunk> _chunkUpdatePos;
	std::map<Vec3d<int>, bool> _updateLoaded;
	
	int _chunkRadius = 4;
	int _renderDist = 4;
	
	bool _worldCreated = false;
	bool _empty = true;
};

namespace wctl_mutex
{
	static std::mutex loadedChunks;
	static std::mutex queueChunks;
	static std::mutex initQueue;
	
	static YanderePool<void (WorldController::*)(Vec3d<int>), WorldController*, Vec3d<int>> cGenPool;
	static YanderePool<void (WorldController::*)(WorldController::UpdateChunk), WorldController*, WorldController::UpdateChunk> cWallPool;
};

#endif
