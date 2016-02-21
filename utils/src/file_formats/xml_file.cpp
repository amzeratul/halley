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

#include "xml_file.h"
#include "tinyxml/ticpp.h"

using namespace Halley;

Halley::XMLFile::XMLFile(String name)
	: filename(name)
	, nFlushes(0)
{
	load();
}

XmlElement* Halley::XMLFile::getRoot() const
{
	return doc->FirstChildElement();
}

void Halley::XMLFile::load()
{
	/*
	auto res = ResourceLocator::getStatic(filename);
	doc = std::shared_ptr<ticpp::Document>(new ticpp::Document());
	doc->Parse(res->getString());
	*/
}
