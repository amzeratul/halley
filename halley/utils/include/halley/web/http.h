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

#ifdef WITH_BOOST_ASIO

#include "../text/halleystring.h"

namespace Halley {
	class HTTPPostEntry {
	public:
		String name;
		String filename;
		String contentType;
		Vector<char> data;
	};

	class HTTP {
	public:
		static Vector<char> get(String host, String path);
		static Vector<char> post(String host, String path, Vector<HTTPPostEntry>& entries);

	private:
		static Vector<char> request(String host, String path, bool isPost, String& content, String boundary);
	};
}

#endif
