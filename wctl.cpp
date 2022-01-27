#include <execution>

#include "wctl.h"

using namespace WorldTypes;

WorldController::WorldController(YandereInitializer* init, Character* mainCharacter, YandereCamera* mainCamera) :
_initer(init), _mainCharacter(mainCharacter), _mainCamera(mainCamera), _empty(false)
{
}

void WorldController::create_world(unsigned seed)
{
	assert(!_empty);
	
	assert(_initer->glew_initialized());
	

	int wallUpdateThreads = 1;
	int chunkLoadThreads = std::thread::hardware_concurrency()-wallUpdateThreads-1;
	
	wctl_mutex::cGenPool = YanderePool(chunkLoadThreads, &WorldController::chunk_loader, this, Vec3d<int>{0, 0, 0});
	wctl_mutex::cWallPool = YanderePool(wallUpdateThreads, &WorldController::update_walls, this, PosWalls{Vec3d<int>{0, 0, 0}});
	

	_worldGen = std::make_unique<WorldGenerator>(_initer, "block_textures");
	
	_worldGen->seed(seed);

	_worldCreated = true;
}


void WorldController::wait_threads()
{
	assert(_worldCreated);
	
	wctl_mutex::cGenPool.exit_threads();
	wctl_mutex::cWallPool.exit_threads();
}

void WorldController::update_walls(PosWalls currChunk)
{
	Vec3d<int>& chunkPos = currChunk.chunkPos;

	std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);
	if(loadedChunks.count(chunkPos)==0)
	{
		std::lock_guard<std::mutex> lockW(wctl_mutex::wallUpdater);
		_chunkUpdateWalls.erase(chunkPos);
		return;
	}
	
	WorldChunk& currChunkPtr = loadedChunks[chunkPos];
	if(currChunkPtr.empty())
		return;
	
	
	if(currChunk.walls.right && loadedChunks.count({chunkPos.x+1, chunkPos.y, chunkPos.z})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x+1, chunkPos.y, chunkPos.z});
	
		if(!checkChunkPtr.empty())
		{
			currChunk.walls.right = false;
			currChunkPtr.update_wall(Direction::right, checkChunkPtr);
		}
	}
		
	if(currChunk.walls.left && loadedChunks.count({chunkPos.x-1, chunkPos.y, chunkPos.z})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x-1, chunkPos.y, chunkPos.z});
	
		if(!checkChunkPtr.empty())
		{
			currChunk.walls.left = false;
			currChunkPtr.update_wall(Direction::left, checkChunkPtr);
		}
	}
		
	if(currChunk.walls.up && loadedChunks.count({chunkPos.x, chunkPos.y+1, chunkPos.z})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x, chunkPos.y+1, chunkPos.z});
	
		if(!checkChunkPtr.empty())
		{
			currChunk.walls.up = false;
			currChunkPtr.update_wall(Direction::up, checkChunkPtr);
		}
	}
		
	if(currChunk.walls.down && loadedChunks.count({chunkPos.x, chunkPos.y-1, chunkPos.z})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x, chunkPos.y-1, chunkPos.z});
	
			if(!checkChunkPtr.empty())
		{
			currChunk.walls.down = false;
			currChunkPtr.update_wall(Direction::down, checkChunkPtr);
		}
	}
			
	if(currChunk.walls.forward && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z+1})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z+1});
		
		if(!checkChunkPtr.empty())
		{
			currChunk.walls.forward = false;
			currChunkPtr.update_wall(Direction::forward, checkChunkPtr);
		}
	}
			
	if(currChunk.walls.back && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z-1})!=0)
	{
		WorldChunk& checkChunkPtr = loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z-1});
		
		if(!checkChunkPtr.empty())
		{
			currChunk.walls.back = false;
			currChunkPtr.update_wall(Direction::back, checkChunkPtr);
		}
	}

	currChunkPtr.apply_model();
			
	if(!currChunk.walls.walls_or())
	{
		std::lock_guard<std::mutex> lockW(wctl_mutex::wallUpdater);
		_chunkUpdateWalls.erase(chunkPos);
	}
}

void WorldController::chunk_loader(const Vec3d<int> chunkPos)
{
	WorldChunk genChunk(_worldGen.get(), chunkPos);
	_worldGen->chunk_gen(genChunk);
	
	std::map<Vec3d<int>, UpdateChunk> queuedMap;
	{
		std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);
	
		if(!genChunk.empty() && genChunk.plants_amount()!=0)
			queuedMap = update_queued();
			
		if(!queuedMap.empty())
			queue_updater(queuedMap);
	}
	
	{
		std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);
		
		genChunk.apply_model();
		
		loadedChunks.emplace(chunkPos, std::move(genChunk));
	}
	
	{
		std::lock_guard<std::mutex> lockI(wctl_mutex::initQueue);
		_initializeChunks.push(chunkPos);
	}
	
	std::lock_guard<std::mutex> lockW(wctl_mutex::wallUpdater);
	_chunkUpdateWalls[chunkPos] = UpdateChunk{};
}

void WorldController::set_visibles()
{
	for(auto& [chunk, info] : _chunksInfo)
	{
		Vec3d<int> diffPos = chunk-_mainCharacter->activeChunkPos;
		
		bool currEmpty = true;
		{
			std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);
			if(loadedChunks.count(chunk)!=0)
				currEmpty = loadedChunks.at(chunk).empty();
		}
		if(currEmpty || Vec3d<int>::magnitude(diffPos) > _renderDist)
		{
			info.visible = false;
		} else
		{
			Vec3d<float> checkPosF = Vec3dCVT<float>(chunk)*chunkSize;
		
			if(_mainCamera->cube_in_frustum({checkPosF.x, checkPosF.y, checkPosF.z}, chunkSize))
			{
				info.visible = true;
			} else
			{
				info.visible = false;
			}
		}
	}
}

bool WorldController::chunk_outside(const Vec3d<int> pos) const
{
	return Vec3d<int>::magnitude(
	Vec3d<int>{std::abs(pos.x), std::abs(pos.y), std::abs(pos.z)}
	- Vec3d<int>{std::abs(_mainCharacter->activeChunkPos.x), std::abs(_mainCharacter->activeChunkPos.y), std::abs(_mainCharacter->activeChunkPos.z)})
	> _chunkRadius+1;
}

void WorldController::range_remove()
{
	//unloads chunks which are outside of a certain range
	{
		std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);
		std::lock_guard<std::mutex> lockI(wctl_mutex::initQueue);
		
		for(auto it = loadedChunks.begin(); it != loadedChunks.end();)
		{
			if(chunk_outside(it->first))
			{
				//unload the chunk
				it->second.remove_model();
				
				_beenLoadedChunks.erase(it->first);
				_chunksInfo.erase(it->first);
					
				it = loadedChunks.erase(it);
			} else
			{
				++it;
			}
		}
	}
}


void WorldController::full_update()
{
	assert(_worldCreated);

	const int renderDiameter = _chunkRadius*2;

		
	for(int x = 0; x < renderDiameter; ++x)
	{
		for(int y = 0; y < renderDiameter; ++y)
		{
			for(int z = 0; z < renderDiameter; ++z)
			{
				Vec3d<int> calcChunk = Vec3d<int>{x-_chunkRadius, y-_chunkRadius, z-_chunkRadius};
				
				if(Vec3d<int>::magnitude(calcChunk) < renderDiameter)
				{
					Vec3d<int> checkChunk = _mainCharacter->activeChunkPos + calcChunk;
						
					if(_beenLoadedChunks.count(checkChunk)==0)
					{
						_beenLoadedChunks.emplace(checkChunk);

						wctl_mutex::cGenPool.run(checkChunk);
					}
				}
			}
		}
	}

	//update chunk meshes to make the block walls continious across chunks
	
	{
		std::lock_guard<std::mutex> lockW(wctl_mutex::wallUpdater);
		for(auto& [chunkPos, walls] : _chunkUpdateWalls)
		{
			wctl_mutex::cWallPool.run(PosWalls{chunkPos, walls});
		}
	}
	
	range_remove();
}

void WorldController::draw_update()
{
	init_chunks();
	
	for(auto& [key, info] : _chunksInfo)
	{
		if(info.visible)
			info.mesh.draw_update();
	}
}

std::map<Vec3d<int>, WorldController::UpdateChunk> WorldController::update_queued()
{
	std::map<Vec3d<int>, UpdateChunk> chunksToUpdate;

	std::lock_guard<std::mutex> lockB(_worldGen->_mtxBlockPlace);
		
	for(auto it = _worldGen->_blockPlaceList.begin(); it != _worldGen->_blockPlaceList.end();)
	{
		WorldGenerator::VecPos& currVecPos = *it;
		const Vec3d<int>& currChunkPos = currVecPos.chunkPos;
	
		if(loadedChunks.count(currChunkPos)!=0)
		{	
			WorldChunk& currChunk = loadedChunks.at(currChunkPos);
				
				
			Direction currDir = Direction::none;

			Vec3d<int> currChunkOffset = currChunkPos - currVecPos.originalPos;
			 
			if(currChunkOffset==Vec3d<int>{1, 0, 0})
				currDir = Direction::right;
						
			if(currChunkOffset==Vec3d<int>{-1, 0, 0})
				currDir = Direction::left;
						
			if(currChunkOffset==Vec3d<int>{0, 1, 0})
				currDir = Direction::up;
						
			if(currChunkOffset==Vec3d<int>{0, -1, 0})
				currDir = Direction::down;
						
			if(currChunkOffset==Vec3d<int>{0, 0, 1})
				currDir = Direction::forward;
						
			if(currChunkOffset==Vec3d<int>{0, 0, -1})
				currDir = Direction::back;
			
			
			currChunk.block(currVecPos.blockPos) = currVecPos.block;

			UpdateChunk* currUpdateChunk;
			if(chunksToUpdate.count(currChunkPos)==0)
			{
				currUpdateChunk = &chunksToUpdate[currChunkPos];
				*currUpdateChunk = UpdateChunk{false, false, false, false, false, false};
			} else
			{
				currUpdateChunk = &chunksToUpdate[currChunkPos];
			}
				
				
			if(currVecPos.blockPos.x==0)
				currUpdateChunk->left = currDir!=Direction::left;
					
			if(currVecPos.blockPos.x==chunkSize-1)
				currUpdateChunk->right = currDir!=Direction::right;
					
			if(currVecPos.blockPos.y==0)
				currUpdateChunk->down = currDir!=Direction::down;
					
			if(currVecPos.blockPos.y==chunkSize-1)
				currUpdateChunk->up = currDir!=Direction::up;
				
			if(currVecPos.blockPos.z==0)
				currUpdateChunk->back = currDir!=Direction::back;
					
			if(currVecPos.blockPos.z==chunkSize-1)
				currUpdateChunk->forward = currDir!=Direction::forward;
				
			
			if(currChunk.empty())
				currChunk.set_empty(currChunk.check_empty());
				
			it = _worldGen->_blockPlaceList.erase(it);
		} else
		{
			if(chunk_outside(currChunkPos))
			{
				it = _worldGen->_blockPlaceList.erase(it);
			} else
			{
				++it;
			}
		}
	}
	
	//is this a useless move? idk
	return std::move(chunksToUpdate);
}

void WorldController::queue_updater(const std::map<Vec3d<int>, UpdateChunk>& queuedChunks)
{
	std::lock_guard<std::mutex> lockW(wctl_mutex::wallUpdater);
	
	for(const auto& [chunkPos, state] : queuedChunks)
	{
		//lol, insanity
		loadedChunks.at(chunkPos).update_mesh();
		loadedChunks.at(chunkPos).apply_model();
		
		_chunkUpdateWalls[chunkPos] = UpdateChunk{};
		
		if(state.left && loadedChunks.count({chunkPos.x-1, chunkPos.y, chunkPos.z})!=0)
		{
			loadedChunks.at({chunkPos.x-1, chunkPos.y, chunkPos.z}).update_mesh();
			loadedChunks.at({chunkPos.x-1, chunkPos.y, chunkPos.z}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x-1, chunkPos.y, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.right && loadedChunks.count({chunkPos.x+1, chunkPos.y, chunkPos.z})!=0)
		{
			loadedChunks.at({chunkPos.x+1, chunkPos.y, chunkPos.z}).update_mesh();
			loadedChunks.at({chunkPos.x+1, chunkPos.y, chunkPos.z}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x+1, chunkPos.y, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.up && loadedChunks.count({chunkPos.x, chunkPos.y+1, chunkPos.z})!=0)
		{
			loadedChunks.at({chunkPos.x, chunkPos.y+1, chunkPos.z}).update_mesh();
			loadedChunks.at({chunkPos.x, chunkPos.y+1, chunkPos.z}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y+1, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.down && loadedChunks.count({chunkPos.x, chunkPos.y-1, chunkPos.z})!=0)
		{
			loadedChunks.at({chunkPos.x, chunkPos.y-1, chunkPos.z}).update_mesh();
			loadedChunks.at({chunkPos.x, chunkPos.y-1, chunkPos.z}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y-1, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.forward && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z+1})!=0)
		{
			loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z+1}).update_mesh();
			loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z+1}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y, chunkPos.z+1}] = UpdateChunk{};
		}
		
		if(state.back && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z-1})!=0)
		{
			loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z-1}).update_mesh();
			loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z-1}).apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y, chunkPos.z-1}] = UpdateChunk{};
		}
	}
}

int WorldController::render_dist()
{
	return _renderDist;
}

int WorldController::chunk_radius()
{
	return _chunkRadius;
}


void WorldController::init_chunks()
{
	assert(_worldCreated);

	int queueSize;
	{
		std::lock_guard<std::mutex> lockI(wctl_mutex::initQueue);
		queueSize = _initializeChunks.size();
	}
	
	for(int i = 0; i < queueSize; ++i)
	{
		Vec3d<int> chunk;
		{
			std::lock_guard<std::mutex> lockI(wctl_mutex::initQueue);
			chunk = _initializeChunks.front();
			
			_initializeChunks.pop();
		}
		
		YanPosition chunkPos;
		{
			std::lock_guard<std::mutex> lockL(wctl_mutex::loadedChunks);	
			
			if(loadedChunks.count(chunk)==0)
				continue;
		}
		
		//casting before multiplying takes more time but allows me to have positions higher than 2 billion (not enough)
		chunkPos = {static_cast<float>(chunk.x)*chunkSize, 
		static_cast<float>(chunk.y)*chunkSize, 
		static_cast<float>(chunk.z)*chunkSize};
	
		YanTransforms chunkT{chunkPos, chunkSize, chunkSize, chunkSize};
		
		_chunksInfo[chunk].mesh = {YandereObject(_initer, WorldChunk::model_name(chunk), _worldGen->atlasName, chunkT)};
	}
	
	if(queueSize>0)
		set_visibles();
}
