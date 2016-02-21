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

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "json_file.h"
#include "json/json.h"

using namespace Halley;


Halley::JSONFile::JSONFile(String name)
	: filename(name)
	, nFlushes(0)
	, root(std::make_unique<JSONValue>())
{
	load();
}

JSONValue& Halley::JSONFile::getRoot()
{
	return *root;
}

void Halley::JSONFile::load()
{
	/*
	auto res = ResourceLocator::getStatic(filename);
	Json::Reader reader;
	reader.parse((const char*)res->getData(), (const char*)res->getData() + res->getSize(), *root);
	*/
}
