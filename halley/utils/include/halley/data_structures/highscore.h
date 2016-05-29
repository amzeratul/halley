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

#include "halley/text/halleystring.h"

namespace Halley {
	typedef std::pair<String, int> HighScoreEntry;

	class HighScore {
	public:
		HighScore(size_t maxEntries);

		const HighScoreEntry& getEntry(size_t n) const;
		size_t getNumEntries() const;
		size_t getMaxEntries() const;
		
		void addScore(String data, int value);
		bool isHighScore(int value) const;

		std::vector<char> save() const;
		void load(std::vector<char>& data);

	private:
		size_t maxEntries;
		std::vector<HighScoreEntry> entries;
	};
}
