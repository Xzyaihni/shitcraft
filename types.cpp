#include <tuple>

#include "types.h"

Direction directionOpposite(Direction dir)
{
	switch(dir)
	{
		case Direction::left:
		return Direction::right;
		
		case Direction::right:
		return Direction::left;
		
		case Direction::up:
		return Direction::down;
		
		case Direction::down:
		return Direction::up;
		
		case Direction::forward:
		return Direction::back;
		
		case Direction::back:
		return Direction::forward;
	
		default:
		return Direction::none;
	}
}

Vec3d<int> directionAdd(Vec3d<int> add_vec, Direction dir, int offset)
{
	switch(dir)
	{
		case Direction::left:
		return {add_vec.x-offset, add_vec.y, add_vec.z};
		
		case Direction::right:
		return {add_vec.x+offset, add_vec.y, add_vec.z};
		
		case Direction::up:
		return {add_vec.x, add_vec.y+offset, add_vec.z};
		
		case Direction::down:
		return {add_vec.x, add_vec.y-offset, add_vec.z};
		
		case Direction::forward:
		return {add_vec.x, add_vec.y, add_vec.z+offset};
		
		case Direction::back:
		return {add_vec.x, add_vec.y, add_vec.z-offset};
	
		default:
		return add_vec;
	}
}
