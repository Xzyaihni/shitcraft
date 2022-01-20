#ifndef CHARACTER_H
#define CHARACTER_H

#include <map>
#include <memory>

#include "physics.h"

class Character : public PhysicsObject
{
public:
	using PhysicsObject::PhysicsObject;
	
	bool onGround = false;
	bool midJump = false;
	bool sneaking = false;
	
	float headHeight = 1;
	
	float moveSpeed = 0;
	float jumpStrength = 1.5f;

private:
	PhysicsController* _physCtl;
};

#endif
