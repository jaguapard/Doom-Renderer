#pragma once
#include <vector>
#include <map>
#include <string>

#include "DoomStructs.h"
#include "DoomMap.h"

class WadLoader
{
public:
	static std::map<std::string, DoomMap> loadWad(std::string path);
};