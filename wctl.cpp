#include "wctl.h"

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
	_worldGen->terrainSmallScale = 1.05f;
	_worldGen->terrainMidScale = 0.22f;
	_worldGen->terrainLargeScale = 0.012f;
	_worldGen->temperatureScale = 0.136f;
	_worldGen->humidityScale = 0.073f;
	
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
	Vec3d<int> chunkPos = currChunk.chunkPos;

	std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);
	if(loadedChunks.count(chunkPos)==0)
		return;
	
	WorldChunk* currChunkPtr = &loadedChunks[chunkPos];
	if(currChunkPtr->empty())
		return;
	
	
	if(loadedChunks.count({chunkPos.x+1, chunkPos.y, chunkPos.z})!=0 && currChunk.walls.right)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x+1, chunkPos.y, chunkPos.z});
	
		if(!checkChunkPtr->empty())
		{
			currChunk.walls.right = false;
			currChunkPtr->update_wall(Direction::right, checkChunkPtr);
		}
	}
		
	if(loadedChunks.count({chunkPos.x-1, chunkPos.y, chunkPos.z})!=0 && currChunk.walls.left)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x-1, chunkPos.y, chunkPos.z});
	
		if(!checkChunkPtr->empty())
		{
			currChunk.walls.left = false;
			currChunkPtr->update_wall(Direction::left, checkChunkPtr);
		}
	}
		
	if(loadedChunks.count({chunkPos.x, chunkPos.y+1, chunkPos.z})!=0 && currChunk.walls.up)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x, chunkPos.y+1, chunkPos.z});
	
		if(!checkChunkPtr->empty())
		{
			currChunk.walls.up = false;
			currChunkPtr->update_wall(Direction::up, checkChunkPtr);
		}
	}
		
	if(loadedChunks.count({chunkPos.x, chunkPos.y-1, chunkPos.z})!=0 && currChunk.walls.down)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x, chunkPos.y-1, chunkPos.z});
	
			if(!checkChunkPtr->empty())
		{
			currChunk.walls.down = false;
			currChunkPtr->update_wall(Direction::down, checkChunkPtr);
		}
	}
			
	if(loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z+1})!=0 && currChunk.walls.forward)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z+1});
		
		if(!checkChunkPtr->empty())
		{
			currChunk.walls.forward = false;
			currChunkPtr->update_wall(Direction::forward, checkChunkPtr);
		}
	}
			
	if(loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z-1})!=0 && currChunk.walls.back)
	{
		WorldChunk* checkChunkPtr = &loadedChunks.at({chunkPos.x, chunkPos.y, chunkPos.z-1});
		
		if(!checkChunkPtr->empty())
		{
			currChunk.walls.back = false;
			currChunkPtr->update_wall(Direction::back, checkChunkPtr);
		}
	}
	
	currChunkPtr->apply_model();
			
	if(!currChunk.walls.walls_or())
	{
		std::unique_lock<std::mutex> lockW(wctl_mutex::wallUpdater);
		_chunkUpdateWalls.erase(chunkPos);
	}
}

void WorldController::chunk_loader(Vec3d<int> chunkPos)
{
	WorldChunk genChunk = WorldChunk(_worldGen.get(), chunkPos);
	genChunk.chunk_gen();
	
	{
		std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);
		
		if(!genChunk.empty())
		{
			update_queued();
			genChunk.apply_model();
		}
		
		loadedChunks[chunkPos] = std::move(genChunk);
	}
	
	{
		std::unique_lock<std::mutex> lockI(wctl_mutex::initQueue);
		_initializeChunks.push(chunkPos);
	}
	
	std::unique_lock<std::mutex> lockW(wctl_mutex::wallUpdater);
	_chunkUpdateWalls[chunkPos] = UpdateChunk{};
}

void WorldController::set_visibles()
{
	for(auto& [chunk, mesh] : _drawMeshes)
	{
		Vec3d<int> diffPos = chunk-_mainCharacter->activeChunkPos;
		
		bool currEmpty;
		{
			std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);
			
			currEmpty = loadedChunks[chunk].empty();
		}
		if(currEmpty || Vec3d<int>::magnitude(diffPos) > _renderDist)
		{
			mesh.visible = false;
		} else
		{
			Vec3d<float> checkPosF = Vec3dCVT<float>(chunk)*chunkSize;
		
			if(_mainCamera->cube_in_frustum({checkPosF.x, checkPosF.y, checkPosF.z}, chunkSize))
			{
				mesh.visible = true;
			} else
			{
				mesh.visible = false;
			}
		}
	}
}

void WorldController::range_remove()
{
	//unloads chunks which are outside of a certain range
	std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);
	
	std::map<Vec3d<int>, WorldChunk>::iterator it;
	for(it = loadedChunks.begin(); it != loadedChunks.end();)
	{
		Vec3d<int> offsetChunk = Vec3d<int>{std::abs(it->first.x), std::abs(it->first.y), std::abs(it->first.z)}
		- Vec3d<int>{std::abs(_mainCharacter->activeChunkPos.x), std::abs(_mainCharacter->activeChunkPos.y), std::abs(_mainCharacter->activeChunkPos.z)};

		if(Vec3d<int>::magnitude(offsetChunk) > _chunkRadius+1)
		{
			//unload the chunk
			it->second.remove_model();
			
			_drawMeshes.erase(it->first);
			
			_updateLoaded[it->first] = false;
				
			it = loadedChunks.erase(it);
		} else
		{
			++it;
		}
	}
}


void WorldController::full_update()
{
	assert(_worldCreated);

	const int renderDiameter = _chunkRadius*2;

	{
		std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);
		
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
						
						if(!_updateLoaded[checkChunk] && loadedChunks.count(checkChunk)==0)
						{
							_updateLoaded[checkChunk] = true;

							wctl_mutex::cGenPool.run(checkChunk);
						}
					}
				}
			}
		}
	}

	//update chunk meshes to make the block walls continious across chunks
	
	{
		std::unique_lock<std::mutex> lockW(wctl_mutex::wallUpdater);
		for(auto& [chunkPos, wallsUpdate] : _chunkUpdateWalls)
		{
			wctl_mutex::cWallPool.run(PosWalls{chunkPos, wallsUpdate});
		}
	}
	
	range_remove();
}

void WorldController::draw_update()
{
	init_chunks();
	
	for(auto& [key, meshVis] : _drawMeshes)
	{
		if(meshVis.visible)
			meshVis.mesh.draw_update();
	}
}

void WorldController::update_queued()
{
	std::map<Vec3d<int>, UpdateChunk> chunkToUpdate;

	std::unique_lock<std::mutex> lockB(_worldGen->_mtxBlockPlace);

	int queueSize = _worldGen->_blockPlaceQueue.size();
	if(queueSize==0)
		return;
	
	for(int i = 0; i < queueSize; ++i)
	{
		WorldGenerator::BlockChunkPos bcPos = _worldGen->_blockPlaceQueue.front();
			
		if(loadedChunks.count(bcPos.chunkPos)==0)
		{
			//can also push to a special map that gets pulled from when loading new chunks
			//2 lazy 2 do that, doesnt seem worth it
			_worldGen->_blockPlaceQueue.push(bcPos);
		} else
		{	
			WorldChunk* currChunk = &loadedChunks[bcPos.chunkPos];
			if(currChunk->empty())
				currChunk->set_empty(false);
			
			WorldBlock* currBlock = &currChunk->block(bcPos.blockPos);
			if(bcPos.replace || currBlock->blockType==WorldTypes::Block::air)
			{
				UpdateChunk* currUpdateChunk;
				if(chunkToUpdate.count(bcPos.chunkPos)==0)
				{
					currUpdateChunk = &chunkToUpdate[bcPos.chunkPos];
					*currUpdateChunk = UpdateChunk{false, false, false, false, false, false};
				} else
				{
					currUpdateChunk = &chunkToUpdate[bcPos.chunkPos];
				}
				
				if(bcPos.blockPos.x==0)
					currUpdateChunk->left = true && bcPos.callChunkSide!=Direction::left;
					
				if(bcPos.blockPos.x==chunkSize-1)
					currUpdateChunk->right = true && bcPos.callChunkSide!=Direction::right;
					
				if(bcPos.blockPos.y==0)
					currUpdateChunk->down = true && bcPos.callChunkSide!=Direction::down;
					
				if(bcPos.blockPos.y==chunkSize-1)
					currUpdateChunk->up = true && bcPos.callChunkSide!=Direction::up;
				
				if(bcPos.blockPos.z==0)
					currUpdateChunk->back = true && bcPos.callChunkSide!=Direction::back;
					
				if(bcPos.blockPos.z==chunkSize-1)
					currUpdateChunk->forward = true && bcPos.callChunkSide!=Direction::forward;
				
				
				*currBlock = bcPos.block;
			}
		}
			
		_worldGen->_blockPlaceQueue.pop();
	}
	
	
	std::unique_lock<std::mutex> lockW(wctl_mutex::wallUpdater);
	
	for(auto& [chunkPos, state] : chunkToUpdate)
	{
		//lol, insanity
		loadedChunks[chunkPos].update_mesh();
		loadedChunks[chunkPos].apply_model();
		
		_chunkUpdateWalls[chunkPos] = UpdateChunk{};
		
		if(state.left && loadedChunks.count({chunkPos.x-1, chunkPos.y, chunkPos.z})!=0)
		{
			loadedChunks[{chunkPos.x-1, chunkPos.y, chunkPos.z}].update_mesh();
			loadedChunks[{chunkPos.x-1, chunkPos.y, chunkPos.z}].apply_model();
			
			_chunkUpdateWalls[{chunkPos.x-1, chunkPos.y, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.right && loadedChunks.count({chunkPos.x+1, chunkPos.y, chunkPos.z})!=0)
		{
			loadedChunks[{chunkPos.x+1, chunkPos.y, chunkPos.z}].update_mesh();
			loadedChunks[{chunkPos.x+1, chunkPos.y, chunkPos.z}].apply_model();
			
			_chunkUpdateWalls[{chunkPos.x+1, chunkPos.y, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.up && loadedChunks.count({chunkPos.x, chunkPos.y+1, chunkPos.z})!=0)
		{
			loadedChunks[{chunkPos.x, chunkPos.y+1, chunkPos.z}].update_mesh();
			loadedChunks[{chunkPos.x, chunkPos.y+1, chunkPos.z}].apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y+1, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.down && loadedChunks.count({chunkPos.x, chunkPos.y-1, chunkPos.z})!=0)
		{
			loadedChunks[{chunkPos.x, chunkPos.y-1, chunkPos.z}].update_mesh();
			loadedChunks[{chunkPos.x, chunkPos.y-1, chunkPos.z}].apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y-1, chunkPos.z}] = UpdateChunk{};
		}
		
		if(state.forward && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z+1})!=0)
		{
			loadedChunks[{chunkPos.x, chunkPos.y, chunkPos.z+1}].update_mesh();
			loadedChunks[{chunkPos.x, chunkPos.y, chunkPos.z+1}].apply_model();
			
			_chunkUpdateWalls[{chunkPos.x, chunkPos.y, chunkPos.z+1}] = UpdateChunk{};
		}
		
		if(state.back && loadedChunks.count({chunkPos.x, chunkPos.y, chunkPos.z-1})!=0)
		{
			loadedChunks[{chunkPos.x, chunkPos.y, chunkPos.z-1}].update_mesh();
			loadedChunks[{chunkPos.x, chunkPos.y, chunkPos.z-1}].apply_model();
			
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
		std::unique_lock<std::mutex> lockI(wctl_mutex::initQueue);
		queueSize = _initializeChunks.size();
	}
	
	for(int i = 0; i < queueSize; ++i)
	{
		Vec3d<int> chunk;
		{
			std::unique_lock<std::mutex> lockI(wctl_mutex::initQueue);
			chunk = _initializeChunks.front();
			
			_initializeChunks.pop();
		}
		
		YanPosition chunkPos;
		{
			std::unique_lock<std::mutex> lockL(wctl_mutex::loadedChunks);	
			
			if(loadedChunks.count(chunk)==0)
				continue;
			
			if(loadedChunks[chunk].empty())
			{
				std::unique_lock<std::mutex> lockI(wctl_mutex::initQueue);
				_initializeChunks.push(chunk);
				continue;
			}
		}
		
		//casting before multiplying takes more time but allows me to have positions higher than 2 billion (not enough)
		chunkPos = {static_cast<float>(chunk.x)*chunkSize, 
		static_cast<float>(chunk.y)*chunkSize, 
		static_cast<float>(chunk.z)*chunkSize};
	
		YanTransforms chunkT{chunkPos, chunkSize, chunkSize, chunkSize};
		
		_drawMeshes[chunk] = {YandereObject(_initer, WorldChunk::model_name(chunk), _worldGen->atlasName, chunkT)};
	}
	
	if(queueSize>0)
		set_visibles();
}
