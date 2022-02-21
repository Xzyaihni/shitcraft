#ifndef WORLDTYPES_H
#define WORLDTYPES_H

#include "types.h"

namespace WorldTypes
{
	struct TexPos
	{
		int x;
		int y;
	};
	
	struct ClimatePoint
	{
		float temperature;
		float humidity;
	};

	enum Biome
	{
		forest = 0,
		desert,
		hell
	};
	
	enum Block
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
	
	struct TextureFace
	{
		WorldTypes::TexPos forward;
		WorldTypes::TexPos back;
		WorldTypes::TexPos right;
		WorldTypes::TexPos left;
		WorldTypes::TexPos up;
		WorldTypes::TexPos down;
	};
	
	struct UpdateChunk
	{
		bool right = true;
		bool left = true;
		bool up = true;
		bool down = true;
		bool forward = true;
		bool back = true;
		
		bool walls_or() {return right || left || up || down || forward || back;};
		void add_walls(UpdateChunk walls)
		{
			right = right||walls.right;
			left = left||walls.left;
			up = up||walls.up;
			down = down||walls.down;
			forward = forward||walls.forward;
			back = back||walls.back;
		};
	};
	
	struct BlockInfo
	{
		bool grassy = false;
	};
	
	constexpr float blockModelSize = 1.0f/static_cast<float>(chunkSize);
};

#endif
