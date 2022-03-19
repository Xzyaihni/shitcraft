#ifndef WORLDTYPES_H
#define WORLDTYPES_H

#include "types.h"

namespace world_types
{
	struct tex_pos
	{
		int x;
		int y;
	};
	
	struct climate_point
	{
		float temperature;
		float humidity;
	};

	enum biome
	{
		forest = 0,
		desert,
		hell
	};
	
	enum block
	{
		air = 0,
		dirt,
		stone,
		sand,
		log,
		leaf,
		cactus,
		lava
	};
	
	struct texture_face
	{
		tex_pos forward;
		tex_pos back;
		tex_pos right;
		tex_pos left;
		tex_pos up;
		tex_pos down;
	};
	
	struct wall_states
	{
		bool right = true;
		bool left = true;
		bool up = true;
		bool down = true;
		bool forward = true;
		bool back = true;
		
		bool walls_or() {return right || left || up || down || forward || back;};
		void add_walls(wall_states walls)
		{
			right = right||walls.right;
			left = left||walls.left;
			up = up||walls.up;
			down = down||walls.down;
			forward = forward||walls.forward;
			back = back||walls.back;
		};
	};
	
	struct block_info
	{
		bool grassy;
	};
	
	const float block_model_size = 1.0f/static_cast<float>(chunk_size);
};

#endif
