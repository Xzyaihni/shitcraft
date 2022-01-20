#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "physics.h"
#include "world.h"

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
			velocity += _physCtl->gravity*timeDelta;
		}

		if(velocity.x!=0||velocity.y!=0||velocity.z!=0)
		{
			Vec3d<float> newPosition;
			newPosition = position+velocity*timeDelta;
		
			if(_physCtl->raycast(position, newPosition).direction!=Direction::none)
			{
				
			} else
			{
				position = newPosition;
			}
			
			bool prevNegativeX = velocity.x<0;
			bool prevNegativeY = velocity.y<0;
			bool prevNegativeZ = velocity.z<0;
			
			velocity -= velocity/_physCtl->friction*timeDelta;
			
			//overshot, correct to 0
			if(prevNegativeX!=(velocity.x<0))
				velocity.x = 0;

			if(prevNegativeY!=(velocity.y<0))
				velocity.y = 0;

			if(prevNegativeZ!=(velocity.z<0))
				velocity.z = 0;

		}
	}
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
	RaycastResult return_result;
	return_result.direction = Direction::none;

	return return_result;
}

Vec3d<float> PhysicsController::calc_dir(float yaw, float pitch)
{
	return {std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::cos(pitch)*std::sin(yaw)};
}
