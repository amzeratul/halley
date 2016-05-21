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

#include "../text/halleystring.h"
//#include "halley_config.h"

#ifdef WITH_BOOST_ASIO

namespace Halley {
	class HTTPPostEntry {
	public:
		String name;
		String filename;
		String contentType;
		std::vector<char> data;
	};

	class HTTP {
	public:
		static std::vector<char> get(String host, String path);
		static std::vector<char> post(String host, String path, std::vector<HTTPPostEntry>& entries);

	private:
		static std::vector<char> request(String host, String path, bool isPost, String& content, String boundary);
	};
}

#endif
