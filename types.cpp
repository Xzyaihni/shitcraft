#include <tuple>

#include "types.h"

ytype::direction direction_opposite(const ytype::direction direction)
{
	switch(direction)
	{
		case ytype::direction::left:
		return ytype::direction::right;
		
		case ytype::direction::right:
		return ytype::direction::left;
		
		case ytype::direction::up:
		return ytype::direction::down;
		
		case ytype::direction::down:
		return ytype::direction::up;
		
		case ytype::direction::forward:
		return ytype::direction::back;
		
		case ytype::direction::back:
		return ytype::direction::forward;
	
		default:
		return ytype::direction::none;
	}
}

vec3d<int> direction_add(const vec3d<int> add_vec, const ytype::direction direction, const int offset)
{
	switch(direction)
	{
		case ytype::direction::left:
		return {add_vec.x-offset, add_vec.y, add_vec.z};
		
		case ytype::direction::right:
		return {add_vec.x+offset, add_vec.y, add_vec.z};
		
		case ytype::direction::up:
		return {add_vec.x, add_vec.y+offset, add_vec.z};
		
		case ytype::direction::down:
		return {add_vec.x, add_vec.y-offset, add_vec.z};
		
		case ytype::direction::forward:
		return {add_vec.x, add_vec.y, add_vec.z+offset};
		
		case ytype::direction::back:
		return {add_vec.x, add_vec.y, add_vec.z-offset};
	
		default:
		return add_vec;
	}
}
