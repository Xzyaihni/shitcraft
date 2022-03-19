#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include <string>

class item
{
public:
	item();

	std::string name;
};

class loot
{
public:
	loot();
	
	std::vector<item> items_vec;
};

#endif
