#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "physics.h"
#include "world.h"


using namespace WorldTypes;

PhysicsObject::PhysicsObject()
{
}

PhysicsObject::PhysicsObject(PhysicsController* physCtl) : _physCtl(physCtl)
{
}

void PhysicsObject::update(double timeDelta)
{
	assert(_physCtl!=nullptr);

	activeChunkPos = WorldChunk::active_chunk(position);

	if(!isStatic)
	{
		if(!floating)
		{
			acceleration += _physCtl->gravity;
		}

		Vec3d<float> newPosition;
		Vec3d<float> newVelocity;

		float crossSectionArea = M_PI*(size*size/4);
		
		Vec3d<float> airResistance = 0.5f*dragCoefficient*_physCtl->airDensity*crossSectionArea*(velocity*velocity);
		airResistance.x = velocity.x>0 ? -airResistance.x : airResistance.x;
		airResistance.y = velocity.y>0 ? -airResistance.y : airResistance.y;
		airResistance.z = velocity.z>0 ? -airResistance.z : airResistance.z;
		
		Vec3d<float> netForceAccel = (force+airResistance)/mass;
		
		newVelocity = velocity + (acceleration+netForceAccel)*timeDelta;
		newPosition = position + velocity*timeDelta;
		
		RaycastResult xRaycast = _physCtl->raycast(position, {newPosition.x, position.y, position.z});
		RaycastResult yRaycast = _physCtl->raycast(position, {position.x, newPosition.y, position.z});
		RaycastResult zRaycast = _physCtl->raycast(position, {position.x, position.y, newPosition.z});
		
		if(xRaycast.direction!=Direction::none)
		{
			newPosition.x = position.x;
			newVelocity.x = 0;
		}
		
		if(yRaycast.direction!=Direction::none)
		{
			newPosition.y = position.y;
			newVelocity.y = 0;
		}

		if(zRaycast.direction!=Direction::none)
		{
			newPosition.z = position.z;
			newVelocity.z = 0;
		}
		
		
		position = newPosition;
		velocity = newVelocity;
	}
}

Vec3d<float> PhysicsObject::apply_friction(Vec3d<float> velocity, float friction)
{
	if(velocity.x>0)
	{
		velocity.x = velocity.x>=friction ? velocity.x - friction : 0;
	} else
	{
		velocity.x = velocity.x<=-friction ? velocity.x + friction : 0;
	}
	
	if(velocity.y>0)
	{
		velocity.y = velocity.y>=friction ? velocity.y - friction : 0;
	} else
	{
		velocity.y = velocity.y<=-friction ? velocity.y + friction : 0;
	}
	
	if(velocity.z>0)
	{
		velocity.z = velocity.z>=friction ? velocity.z - friction : 0;
	} else
	{
		velocity.z = velocity.z<=-friction ? velocity.z + friction : 0;
	}
	
	return velocity;
}


PhysicsController::PhysicsController()
{
}

PhysicsController::PhysicsController(std::map<Vec3d<int>, WorldChunk>* worldChunks) : _worldChunks(worldChunks)
{
}

void PhysicsController::physics_update(double timeDelta)
{
	std::for_each(physObjs.begin(), physObjs.end(), [timeDelta](auto& obj){obj.get().update(timeDelta);});
}


RaycastResult PhysicsController::raycast(Vec3d<float> startPos, Vec3d<float> endPos)
{
	Vec3d<int> vecDifference = WorldChunk::closest_block(endPos) - WorldChunk::closest_block(startPos);
	
	int length = std::abs(vecDifference.x)+std::abs(vecDifference.y)+std::abs(vecDifference.z);
	
	if(length==0)
		return RaycastResult{Direction::none, {0, 0, 0}, {0, 0, 0}};
	
	return raycast(startPos, endPos-startPos, length);
}

RaycastResult PhysicsController::raycast(Vec3d<float> startPos, Vec3d<float> direction, int length)
{
	RaycastResult rayResult;
	
	Vec3d<int> currChunkPos = WorldChunk::active_chunk(startPos);
	
	Vec3d<float> startBlockPos = Vec3d<float>{static_cast<float>(std::fmod(startPos.x, chunkSize)), static_cast<float>(std::fmod(startPos.y, chunkSize)), static_cast<float>(std::fmod(startPos.z, chunkSize))};
	startBlockPos.x = startPos.x<0 ? chunkSize+startBlockPos.x : startBlockPos.x;
	startBlockPos.y = startPos.y<0 ? chunkSize+startBlockPos.y : startBlockPos.y;
	startBlockPos.z = startPos.z<0 ? chunkSize+startBlockPos.z : startBlockPos.z;
	
	direction /= Vec3d<float>::magnitude(direction); //normalize it
	Vec3d<float> directionAbs{std::abs(direction.x), std::abs(direction.y), std::abs(direction.z)};
	
	Vec3d<int> currCheckPos = WorldChunk::closest_block(startBlockPos);
	Vec3d<bool> moveSide{false, false, false};
	
	
	float temp;
	
	Vec3d<float> fractionalPos{std::abs(std::modf(startPos.x, &temp)), std::abs(std::modf(startPos.y, &temp)), std::abs(std::modf(startPos.z, &temp))};
	
	Vec3d<float> positionChange;
	if(startPos.x<0)
	{
		positionChange.x = direction.x<0 ? fractionalPos.x : 1-fractionalPos.x;
	} else
	{
		positionChange.x = direction.x<0 ? 1-fractionalPos.x : fractionalPos.x;
	}
	
	if(startPos.y<0)
	{
		positionChange.y = direction.y<0 ? fractionalPos.y : 1-fractionalPos.y;
	} else
	{
		positionChange.y = direction.y<0 ? 1-fractionalPos.y : fractionalPos.y;
	}
	
	if(startPos.z<0)
	{
		positionChange.z = direction.z<0 ? fractionalPos.z : 1-fractionalPos.z;
	} else
	{
		positionChange.z = direction.z<0 ? 1-fractionalPos.z : fractionalPos.z;
	}
	
	while(true)
	{
		auto currIter = _worldChunks->find(currChunkPos);
		if(currIter==_worldChunks->end())
			return RaycastResult{Direction::none, {0, 0, 0}, {0, 0, 0}};
	
		while(true)
		{
			if(currIter->second.block(currCheckPos).blockType!=Block::air)
				return RaycastResult{(moveSide.x?(direction.x<0? Direction::left : Direction::right):(moveSide.y?(direction.y<0? Direction::down : Direction::up):(direction.z<0? Direction::back : Direction::forward))), currChunkPos, currCheckPos};
			
			if(length==0)
				return RaycastResult{Direction::none, {0, 0, 0}, {0, 0, 0}};
			
			Vec3d<float> wallDist;
			
			wallDist.x = direction.x==0 ? INFINITY : (1-positionChange.x)/directionAbs.x;
			wallDist.y = direction.y==0 ? INFINITY : (1-positionChange.y)/directionAbs.y;
			wallDist.z = direction.z==0 ? INFINITY : (1-positionChange.z)/directionAbs.z;
			
						
			--length;
			if(wallDist.x <= wallDist.y && wallDist.x <= wallDist.z)
			{
				//x wall is closest
				positionChange.x = 0;
				
				positionChange.y += std::abs(wallDist.x*direction.y);
				positionChange.y = positionChange.y>1 ? 1 : positionChange.y;
				
				positionChange.z += std::abs(wallDist.x*direction.z);
				positionChange.z = positionChange.z>1 ? 1 : positionChange.z;
				
				moveSide = {true, false, false};
				if(direction.x<0)
				{
					if(currCheckPos.x==0)
					{
						currCheckPos.x = chunkSize-1;
						--currChunkPos.x;
						break;
					} else
					{
						--currCheckPos.x;
					}
				} else 
				{
					if(currCheckPos.x==chunkSize-1)
					{
						currCheckPos.x = 0;
						++currChunkPos.x;
						break;
					} else
					{
						++currCheckPos.x;
					}
				}
			} else if(wallDist.y <= wallDist.x && wallDist.y <= wallDist.z)
			{
				//y wall is closest
				positionChange.x += std::abs(wallDist.y*direction.x);
				positionChange.x = positionChange.x>1 ? 1 : positionChange.x;
				
				positionChange.y = 0;
				
				positionChange.z += std::abs(wallDist.y*direction.z);
				positionChange.z = positionChange.z>1 ? 1 : positionChange.z;
				
				moveSide = {false, true, false};
				if(direction.y<0)
				{
					if(currCheckPos.y==0)
					{
						currCheckPos.y = chunkSize-1;
						--currChunkPos.y;
						break;
					} else
					{
						--currCheckPos.y;
					}
				} else 
				{
					if(currCheckPos.y==chunkSize-1)
					{
						currCheckPos.y = 0;
						++currChunkPos.y;
						break;
					} else
					{
						++currCheckPos.y;
					}
				}
			} else
			{
				//z wall is closest
				positionChange.x += std::abs(wallDist.z*direction.x);
				positionChange.x = positionChange.x>1 ? 1 : positionChange.x;
				
				positionChange.y += std::abs(wallDist.z*direction.y);
				positionChange.y = positionChange.y>1 ? 1 : positionChange.y;
				
				positionChange.z = 0;
				
				moveSide = {false, false, true};
				if(direction.z<0)
				{
					if(currCheckPos.z==0)
					{
						currCheckPos.z = chunkSize-1;
						--currChunkPos.z;
						break;
					} else
					{
						--currCheckPos.z;
					}
				} else 
				{
					if(currCheckPos.z==chunkSize-1)
					{
						currCheckPos.z = 0;
						++currChunkPos.z;
						break;
					} else
					{
						++currCheckPos.z;
					}
				}
			}
		}
	}
}

int PhysicsController::raycast_distance(Vec3d<float> rayStart, RaycastResult raycast)
{
	Vec3d<float> chunkOffset = Vec3dCVT<float>((raycast.chunk-WorldChunk::active_chunk(rayStart))*chunkSize);
	
	Vec3d<float> inChunkPos{
	static_cast<float>(std::fmod(rayStart.x, chunkSize)),
	static_cast<float>(std::fmod(rayStart.y, chunkSize)),
	static_cast<float>(std::fmod(rayStart.z, chunkSize))};
	
	inChunkPos.x = inChunkPos.x<0 ? chunkSize+inChunkPos.x : inChunkPos.x;
	inChunkPos.y = inChunkPos.y<0 ? chunkSize+inChunkPos.y : inChunkPos.y;
	inChunkPos.z = inChunkPos.z<0 ? chunkSize+inChunkPos.z : inChunkPos.z;
	
	chunkOffset = chunkOffset+Vec3dCVT<float>(raycast.block)-inChunkPos;

	return Vec3d<float>::magnitude(chunkOffset);
}

Vec3d<float> PhysicsController::calc_dir(float yaw, float pitch)
{
	return {std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::cos(pitch)*std::sin(yaw)};
}
