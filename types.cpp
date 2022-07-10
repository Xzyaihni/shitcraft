#include <tuple>

#include "types.h"


using namespace ytype;

direction ytype::direction_opposite(const direction direction)
{
	switch(direction)
	{
		case direction::left:
		return direction::right;
		
		case direction::right:
		return direction::left;
		
		case direction::up:
		return direction::down;
		
		case direction::down:
		return direction::up;
		
		case direction::forward:
		return direction::back;
		
		case direction::back:
		return direction::forward;
	
		default:
		return direction::none;
	}
}

vec3d<int> ytype::direction_offset(const direction direction)
{
	switch(direction)
	{
		case direction::left:
		return vec3d<int>{-1, 0, 0};

		case direction::right:
		return vec3d<int>{1, 0, 0};

		case direction::up:
		return vec3d<int>{0, 1, 0};

		case direction::down:
		return vec3d<int>{0, -1, 0};

		case direction::forward:
		return vec3d<int>{0, 0, 1};

		case direction::back:
		return vec3d<int>{0, 0, -1};

		default:
		return vec3d<int>{0, 0, 0};
	}
}

vec3d<int> ytype::direction_add(const vec3d<int> add_vec, const direction direction, const int offset)
{
	return add_vec+direction_offset(direction)*offset;
}

int ytype::round_away(const float val) noexcept
{
	return val<0?static_cast<int>(std::floor(val)):static_cast<int>(std::ceil(val));
}
