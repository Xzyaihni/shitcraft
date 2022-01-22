#ifndef WBLOCK_H
#define WBLOCK_H

#include "worldtypes.h"
#include "inventory.h"


struct WorldBlock
{
public:
	int blockType;	
	WorldTypes::BlockInfo info = {}; 

	void update();
	Loot destroy();
	
	WorldTypes::TextureFace texture();
	bool transparent();
};


#endif
