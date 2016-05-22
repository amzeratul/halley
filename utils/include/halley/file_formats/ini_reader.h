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

#include "halley/text/halleystring.h"
#include <map>

namespace Halley {
	class INIHandle {
	public:
		INIHandle(String str);

		operator String() const;
		operator int() const;
		operator float() const;

	private:
		String data;
	};

	class INIFile {
	public:
		INIFile(String data);
		void parse(String data);

		String getString(String key) const;
		int getInt(String key) const;
		float getFloat(String key) const;
		INIHandle get(String key) const;

	private:
		String filename;
		std::map<String, String> entries;
	};
}
