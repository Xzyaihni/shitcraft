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
		
		if(_physCtl->raycast(position, newPosition).direction!=Direction::none)
		{
				
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
	RaycastResult return_result;
	return_result.direction = Direction::none;

	return return_result;
}

Vec3d<float> PhysicsController::calc_dir(float yaw, float pitch)
{
	return {std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::cos(pitch)*std::sin(yaw)};
}
