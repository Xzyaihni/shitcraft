#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include <string>

class Item
{
public:
	Item();

	std::string name;
};

class Loot
{
public:
	Loot();
	
	std::vector<Item> itemsList;
};

#endif
