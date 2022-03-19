#include "wblock.h"

using namespace world_types;


void world_block::update()
{
}

loot world_block::destroy()
{
	block_type = block::air;

	return loot{};
}

texture_face world_block::texture() const
{
	switch(block_type)
	{
		case block::dirt:
			return info.grassy ? texture_face{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 1}, {0, 2}}
			: texture_face{{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}};
			
		case block::stone:
			return texture_face{{1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}};
			
		case block::sand:
			return texture_face{{2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}};
			
		case block::log:
			return texture_face{{3, 0}, {3, 0}, {3, 0}, {3, 0}, {3, 1}, {3, 1}};
			
		case block::leaf:
			return texture_face{{4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}};
			
		case block::cactus:
			return texture_face{{5, 0}, {5, 0}, {5, 0}, {5, 0}, {5, 1}, {5, 1}};
			
		case block::lava:
			return texture_face{{6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}};
		
		default:
			return texture_face{};
	}
}

bool world_block::transparent() const
{
	switch(block_type)
	{
		case block::leaf:
		case block::air:
			return true;
		
		default:
			return false;
	}
}
