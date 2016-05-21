/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "ini_reader.h"
#include <memory>
#include <cstring>
#include "../support/exception.h"
using namespace Halley;

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif


INIFile::INIFile(String data) {
	parse(data);
}

void Halley::INIFile::parse(String data)
{
	//std::cout << "Parsing ini data: [[\n" << data << "]]\n";

	entries.clear();
	if (data != "") {
		char* str = &data[0];
		char* cur = strtok(str, "\n");
		String group = "";
		while (cur != nullptr) {
			String line = cur;
			line.trimBoth();
			if (line != "") {
				if (line.startsWith("[") && line.endsWith("]")) {
					group = line.mid(1, line.length()-2);
				} else if (!line.startsWith("#") && line.contains("=")) {
					size_t pos = line.find('=');
					String key = line.left(pos).asciiLower();
					String value = line.mid(pos+1);
					key.trimBoth();
					value.trimBoth();
					entries[group+"."+key] = value;
				}
			}

			cur = strtok(nullptr, "\n");
		}
	}
}

Halley::String Halley::INIFile::getString(String key) const
{
	auto result = entries.find(key.asciiLower());
	if (result != entries.end()) return result->second;
	throw Exception("Key \""+key+"\" not found (" + String::integerToString((int)entries.size()) +" keys loaded).");
}

int Halley::INIFile::getInt(String key) const
{
	return getString(key).toInteger();
}

float Halley::INIFile::getFloat(String key) const
{
	return getString(key).toFloat();
}

Halley::INIHandle Halley::INIFile::get(String key) const
{
	return INIHandle(getString(key));
}

Halley::INIHandle::INIHandle(String str)
	: data(str)
{
}

INIHandle::operator String() const
{
	return data;
}

INIHandle::operator int() const
{
	return data.toInteger();
}

INIHandle::operator float() const
{
	return data.toFloat();
}
