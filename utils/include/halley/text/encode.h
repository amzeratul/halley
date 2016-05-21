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

#pragma once

#include "halleystring.h"

namespace Halley {
	namespace Encode {
		// I hope you guys have an r-value reference aware compiler

		String encodeBase64(const std::vector<char>& in);
		std::vector<char> decodeBase64(const String& in);

		std::vector<char> encodeRLE(const std::vector<char>& in);
		std::vector<char> decodeRLE(const std::vector<char>& in);
	}
}