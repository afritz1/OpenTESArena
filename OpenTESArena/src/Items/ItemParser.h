#ifndef ITEM_PARSER_H
#define ITEM_PARSER_H

#include <memory>
#include <vector>

class Item;

class ItemParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;

	ItemParser() = delete;
	ItemParser(const ItemParser&) = delete;
	~ItemParser() = delete;
public:
	static std::vector<std::unique_ptr<Item>> parse();
};

#endif
