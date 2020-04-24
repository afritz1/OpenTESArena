#ifndef MAP_INFO_DEFINITION_H
#define MAP_INFO_DEFINITION_H

// Modern replacement for .INF files.

class INFFile;

class MapInfoDefinition
{
private:
	// @todo: texture names, texture mappings, entity textures, keys, riddles, etc..
public:
	void init(const INFFile &inf);
};

#endif
