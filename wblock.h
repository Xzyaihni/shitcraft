#ifndef WBLOCK_H
#define WBLOCK_H

#include "worldtypes.h"
#include "inventory.h"


struct world_block
{
	int block_type;	
	world_types::block_info info; 

	void update();
	loot destroy();
	
	world_types::texture_face texture() const;
	bool transparent() const;
};

namespace world_types
{
	struct block_place
	{
		vec3d<int> pos;
			
		world_block block;
	};
}

#endif
