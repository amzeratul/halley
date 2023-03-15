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

#include "halley/text/halleystring.h"
#include "halley/file_formats/text_reader.h"
#include <fstream>
#include <algorithm>
using namespace Halley;

StringArray TextReader::ReadFile(String filename)
{
	StringArray result;
	std::ifstream stream(filename.c_str(), std::ios::in);
	if (stream.is_open()) {
		std::string line;
		while (!stream.eof()) {
			getline(stream, line);
			result.push_back(line);
		}
	}

	return result;
}
