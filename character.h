#ifndef CHARACTER_H
#define CHARACTER_H

#include <map>
#include <memory>

#include "physics.h"

class character : public physics_object
{
public:
	using physics_object::physics_object;
	
	bool on_ground = false;
	bool mid_jump = false;
	bool sneaking = false;
	
	float head_height = 1;
	
	float move_speed = 0;
	float jump_strength = 1.5f;

private:
	physics_controller* _phys_ctl;
};

#endif
