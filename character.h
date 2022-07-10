#ifndef CHARACTER_H
#define CHARACTER_H

#include <map>
#include <memory>

#include "physics.h"

class character : public physics::object
{
public:
	using physics::object::object;
	
	bool on_ground = false;
	bool mid_jump = false;
	bool sneaking = false;
	
	float head_height = 1;
	
	float move_speed = 0;
	float jump_strength = 1.5f;
};

#endif
