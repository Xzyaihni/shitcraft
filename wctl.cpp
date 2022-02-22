#include <execution>

#include "wctl.h"

using namespace WorldTypes;

WorldController::WorldController(YandereInitializer* init, Character* mainCharacter, YandereCamera* mainCamera) :
_initer(init), _mainCharacter(mainCharacter), _mainCamera(mainCamera), _empty(false)
{
}

void WorldController::create_world(unsigned blockTexturesID, unsigned seed)
{
	assert(!_empty);
	
	assert(_initer->glew_initialized());
	
	int maxThreads = std::thread::hardware_concurrency()-1;
	float chunkLoadThreadsPercent = 1; //100% of threads to chunkLoadThreads
	
	int chunkLoadThreads = maxThreads*chunkLoadThreadsPercent;
	
	_chunkGenPool = std::make_unique<poolType>(chunkLoadThreads, &WorldController::chunk_loader, this, Vec3d<int>{0, 0, 0});
	

	_worldGen = std::make_unique<WorldGenerator>(_initer, blockTexturesID);
	
	_worldGen->seed(seed);

	_worldCreated = true;
}


void WorldController::chunk_update(WorldChunk& chunk)
{
	chunk.update_mesh();
	update_walls(_chunkUpdateWalls.insert_or_assign(chunk.position(), UpdateChunk{}).first);
}

void WorldController::chunk_update_full(WorldChunk& chunk, Vec3d<int> blockPos)
{
	chunk_update_full(chunk, UpdateChunk{blockPos.x==chunkSize-1, blockPos.x==0, blockPos.y==chunkSize-1, blockPos.y==0, blockPos.z==chunkSize-1, blockPos.z==0});
}

void WorldController::chunk_update_full(WorldChunk& chunk, UpdateChunk walls)
{
	Vec3d<int>& pos = chunk.position();

	chunk_update(chunk);
	
	if(walls.left)
	{
		auto leftIter = worldChunks.find({pos.x-1, pos.y, pos.z});
		if(leftIter!=worldChunks.end())
			chunk_update(leftIter->second);
	}
	
	if(walls.right)
	{
		auto rightIter = worldChunks.find({pos.x+1, pos.y, pos.z});
		if(rightIter!=worldChunks.end())
			chunk_update(rightIter->second);
	}
	
	if(walls.down)
	{
		auto downIter = worldChunks.find({pos.x, pos.y-1, pos.z});
		if(downIter!=worldChunks.end())
			chunk_update(downIter->second);
	}
	
	if(walls.up)
	{
		auto upIter = worldChunks.find({pos.x, pos.y+1, pos.z});
		if(upIter!=worldChunks.end())
			chunk_update(upIter->second);
	}
		
	if(walls.back)
	{
		auto backIter = worldChunks.find({pos.x, pos.y, pos.z-1}); 	
		if(backIter!=worldChunks.end())
			chunk_update(backIter->second);
	}
		
	if(walls.forward)
	{
		auto forwardIter = worldChunks.find({pos.x, pos.y, pos.z+1});
		if(forwardIter!=worldChunks.end())
			chunk_update(forwardIter->second);
	}
}


void WorldController::wait_threads()
{
	assert(_worldCreated);
	
	_chunkGenPool->exit_threads();
}

void WorldController::update_walls(std::map<Vec3d<int>, UpdateChunk>::iterator iter)
{
	const Vec3d<int>& chunkPos = iter->first;
	UpdateChunk& walls = iter->second;

	auto currIter = worldChunks.find(chunkPos);
	if(currIter==worldChunks.end())
		return;
	
	WorldChunk& currChunk = currIter->second;
	
	auto rightIter = worldChunks.find({chunkPos.x+1, chunkPos.y, chunkPos.z});
	auto leftIter = worldChunks.find({chunkPos.x-1, chunkPos.y, chunkPos.z});
	auto upIter = worldChunks.find({chunkPos.x, chunkPos.y+1, chunkPos.z});
	auto downIter = worldChunks.find({chunkPos.x, chunkPos.y-1, chunkPos.z});
	auto forwardIter = worldChunks.find({chunkPos.x, chunkPos.y, chunkPos.z+1});
	auto backIter = worldChunks.find({chunkPos.x, chunkPos.y, chunkPos.z-1});
	
	
	if(leftIter!=worldChunks.end())
	{
		auto leftWallsIter = _chunkUpdateWalls.find({chunkPos.x-1, chunkPos.y, chunkPos.z});
		
		if(leftWallsIter!=_chunkUpdateWalls.end() && leftWallsIter->second.right)
		{
			leftWallsIter->second.right = false;
			leftIter->second.update_wall(Direction::right, currChunk);
		}
	}
	
	if(rightIter!=worldChunks.end())
	{
		auto rightWallsIter = _chunkUpdateWalls.find({chunkPos.x+1, chunkPos.y, chunkPos.z});
		
		if(rightWallsIter!=_chunkUpdateWalls.end() && rightWallsIter->second.left)
		{
			rightWallsIter->second.left = false;
			rightIter->second.update_wall(Direction::left, currChunk);
		}
	}
	
	if(downIter!=worldChunks.end())
	{
		auto downWallsIter = _chunkUpdateWalls.find({chunkPos.x, chunkPos.y-1, chunkPos.z});
		
		if(downWallsIter!=_chunkUpdateWalls.end() && downWallsIter->second.up)
		{
			downWallsIter->second.up = false;
			downIter->second.update_wall(Direction::up, currChunk);
		}
	}
	
	if(upIter!=worldChunks.end())
	{
		auto upWallsIter = _chunkUpdateWalls.find({chunkPos.x, chunkPos.y+1, chunkPos.z});
		
		if(upWallsIter!=_chunkUpdateWalls.end() && upWallsIter->second.down)
		{
			upWallsIter->second.down = false;
			upIter->second.update_wall(Direction::down, currChunk);
		}
	}
	
	if(backIter!=worldChunks.end())
	{
		auto backWallsIter = _chunkUpdateWalls.find({chunkPos.x, chunkPos.y, chunkPos.z-1});
		
		if(backWallsIter!=_chunkUpdateWalls.end() && backWallsIter->second.forward)
		{
			backWallsIter->second.forward = false;
			backIter->second.update_wall(Direction::forward, currChunk);
		}
	}
	
	if(forwardIter!=worldChunks.end())
	{
		auto forwardWallsIter = _chunkUpdateWalls.find({chunkPos.x, chunkPos.y, chunkPos.z+1});
		
		if(forwardWallsIter!=_chunkUpdateWalls.end() && forwardWallsIter->second.back)
		{
			forwardWallsIter->second.back = false;
			forwardIter->second.update_wall(Direction::back, currChunk);
		}
	}
	
	
	if(currChunk.empty())
	{
		_chunkUpdateWalls.erase(iter);
		return;
	}
	
	//right wall
	if(walls.right && rightIter!=worldChunks.end())
	{
		walls.right = false;
		currChunk.update_wall(Direction::right, rightIter->second);
	}
		
	//left wall
	if(walls.left && leftIter!=worldChunks.end())
	{
		walls.left = false;
		currChunk.update_wall(Direction::left, leftIter->second);
	}
	
	//up wall
	if(walls.up && upIter!=worldChunks.end())
	{
		walls.up = false;
		currChunk.update_wall(Direction::up, upIter->second);
	}
	
	//down wall
	if(walls.down && downIter!=worldChunks.end())
	{
		walls.down = false;
		currChunk.update_wall(Direction::down, downIter->second);
	}
	
	//forward wall
	if(walls.forward && forwardIter!=worldChunks.end())
	{
		walls.forward = false;
		currChunk.update_wall(Direction::forward, forwardIter->second);
	}
	
	//back wall
	if(walls.back && backIter!=worldChunks.end())
	{
		walls.back = false;
		currChunk.update_wall(Direction::back, backIter->second);
	}
	
	
	if(!walls.walls_or())
		_chunkUpdateWalls.erase(iter);
}

void WorldController::chunk_loader(const Vec3d<int> chunkPos)
{
	unsigned chunkModelID;
		
	WorldChunk genChunk(_worldGen.get(), chunkPos);
	_worldGen->chunk_gen(genChunk);
	
	std::lock_guard<std::mutex> lockL(_loadChunksMtx);
		
	_chunksLoad.reserve(1);
	_chunksLoad.emplace_back(std::move(genChunk));
}

void WorldController::set_visibles()
{
	for(auto& [chunk, info] : _chunksInfo)
	{
		//frustum culling
		
		Vec3d<int> diffPos = chunk-_mainCharacter->activeChunkPos;
		
		auto currIter = worldChunks.find(chunk);
		
		bool currEmpty = currIter==worldChunks.end() || currIter->second.empty();
			
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
		
		
		//todo occlusion culling
	}
}

float WorldController::chunk_outside(const Vec3d<int> pos) const
{
	return Vec3d<int>::magnitude(
	Vec3d<int>{std::abs(pos.x), std::abs(pos.y), std::abs(pos.z)}
	- Vec3d<int>{std::abs(_mainCharacter->activeChunkPos.x), std::abs(_mainCharacter->activeChunkPos.y), std::abs(_mainCharacter->activeChunkPos.z)});
}

void WorldController::range_remove()
{
	//unloads chunks which are outside of a certain range
	for(auto it = worldChunks.begin(); it != worldChunks.end();)
	{
		if(chunk_outside(it->first) > _chunkRadius+1)
		{
			//unload the chunk
			it->second.remove_model();
			
			_beenLoadedChunks.erase(it->first);
			_chunksInfo.erase(it->first);
				
			it = worldChunks.erase(it);
		} else
		{
			++it;
		}
	}
}

void WorldController::add_chunks()
{
	bool updateShared = false;

	int chunksLoadSize = 0;

	{
		std::lock_guard<std::mutex> lockL(_loadChunksMtx);
		
		chunksLoadSize = _chunksLoad.size();
		
		for(auto& chunk : _chunksLoad)
		{
			Vec3d<int>& chunkPos = chunk.position();
	
			updateShared = updateShared || (chunk.plants_amount()!=0);
	
			auto iter = _queuedBlocks.find(chunkPos);
			if(!chunk.empty() && iter!=_queuedBlocks.end())
			{
				chunk_update_full(chunk, iter->second);
				_queuedBlocks.erase(chunkPos);
			}
	
			chunk.create_mesh();
			_initializeChunks.push(chunkPos);
			worldChunks.emplace(chunkPos, std::move(chunk));

			update_walls(_chunkUpdateWalls.emplace(chunkPos, UpdateChunk{}).first);
		}
		
		_chunksLoad.clear();
	}
	
	if(updateShared)
		update_queued(_queuedBlocks);
	
	if(chunksLoadSize!=0)
	{
		//update out of chunk blocks if any chunks were generated
		for(auto it = _queuedBlocks.begin(); it!=_queuedBlocks.end();)
		{
			auto chunkIter = worldChunks.find(it->first);
			if(chunkIter!=worldChunks.end())
			{
				chunk_update_full(chunkIter->second, it->second);
				
				it = _queuedBlocks.erase(it);
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
				
				if(Vec3d<int>::magnitude(calcChunk) < _chunkRadius)
				{
					Vec3d<int> checkChunk = _mainCharacter->activeChunkPos + calcChunk;
						
					if(_beenLoadedChunks.insert(checkChunk).second)
						_chunkGenPool->run(checkChunk);
				}
			}
		}
	}
	
	range_remove();
	add_chunks();
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

void WorldController::update_queued(std::map<Vec3d<int>, UpdateChunk>& updateChunks)
{
	std::lock_guard<std::mutex> lockB(_worldGen->_mtxBlockPlace);
		
	for(auto it = _worldGen->_blockPlaceVec.begin(); it != _worldGen->_blockPlaceVec.end();)
	{
		WorldGenerator::VecPos& currVecPos = *it;
		const Vec3d<int>& currChunkPos = currVecPos.chunkPos;
	
		auto currIter = worldChunks.find(currChunkPos);
		if(currIter!=worldChunks.end())
		{	
			WorldChunk& currChunk = currIter->second;
				
			currChunk.block(currVecPos.blockPos) = currVecPos.block;
			
			auto updateIter = updateChunks.try_emplace(currChunkPos, UpdateChunk{false, false, false, false, false, false});
			
			updateIter.first->second.add_walls(currVecPos.walls);
			
			if(currChunk.empty())
				currChunk.set_empty(currChunk.check_empty());
				
		} else
		{
			if(chunk_outside(currChunkPos) <= _chunkRadius+2)
			{
				++it;
				continue;
			}
		}
		
		auto newVecEnd = _worldGen->_blockPlaceVec.end()-1;
		
		if(newVecEnd!=it)
			*it = std::move(*newVecEnd);
			
		_worldGen->_blockPlaceVec.pop_back();
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

	bool empty = _initializeChunks.empty();

	while(!_initializeChunks.empty())
	{
		Vec3d<int> chunk = _initializeChunks.front();
		
		_initializeChunks.pop();
		
		auto currIter = worldChunks.find(chunk);
		if(currIter==worldChunks.end())
			continue;
		
		//casting before multiplying takes more time but allows me to have positions higher than 2 billion (not enough)
		YanPosition chunkPos = {static_cast<float>(chunk.x)*chunkSize, 
		static_cast<float>(chunk.y)*chunkSize, 
		static_cast<float>(chunk.z)*chunkSize};
	
		YanTransforms chunkT{chunkPos, chunkSize, chunkSize, chunkSize};
		
		_chunksInfo[chunk].mesh = {YandereObject(_initer, currIter->second.modelID(), _worldGen->atlasTextureID, chunkT)};
	}
	
	if(!empty)
		set_visibles();
}
