#ifndef WBLOCK_H
#define WBLOCK_H

#include "worldtypes.h"
#include "inventory.h"


struct WorldBlock
{
	int blockType;	
	WorldTypes::BlockInfo info = {}; 

	void update();
	Loot destroy();
	
	WorldTypes::TextureFace texture() const;
	bool transparent() const;
};

namespace WorldTypes
{
	struct BlockPlace
	{
		Vec3d<int> pos;
			
		WorldBlock block;
	};
}

#endif
