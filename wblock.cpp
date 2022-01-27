#include "wblock.h"

using namespace WorldTypes;


void WorldBlock::update()
{
}

Loot WorldBlock::destroy()
{
	return Loot{};
}

TextureFace WorldBlock::texture() const
{
	switch(blockType)
	{
		case Block::dirt:
			return info.grassy ? TextureFace{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 1}, {0, 2}}
			: TextureFace{{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}};
			
		case Block::stone:
			return TextureFace{{1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}};
			
		case Block::sand:
			return TextureFace{{2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 0}};
			
		case Block::log:
			return TextureFace{{3, 0}, {3, 0}, {3, 0}, {3, 0}, {3, 1}, {3, 1}};
			
		case Block::leaf:
			return TextureFace{{4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}};
			
		case Block::cactus:
			return TextureFace{{5, 0}, {5, 0}, {5, 0}, {5, 0}, {5, 1}, {5, 1}};
			
		case Block::lava:
			return TextureFace{{6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}};
		
		default:
			return TextureFace{};
	}
}

bool WorldBlock::transparent() const
{
	switch(blockType)
	{
		case Block::leaf:
		case Block::air:
			return true;
		
		default:
			return false;
	}
}
