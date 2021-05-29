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

  Copyright (c) 2007-2012 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <halley/text/halleystring.h>
#include <halley/utils/utils.h>

namespace Halley {
	class HTTPPostEntry {
	public:
		String name;
		String filename;
		String contentType;
		Bytes data;
	};

	class HTTP {
	public:
		static Bytes request(const String& host, const String& path, const String & method, const String& content, const std::map<String, String>& headers);
	};
}
