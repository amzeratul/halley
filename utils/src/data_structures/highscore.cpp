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

#include "highscore.h"
using namespace Halley;


Halley::HighScore::HighScore(size_t m)
	: maxEntries(m)
{
}

const HighScoreEntry& Halley::HighScore::getEntry(size_t n) const
{
	return entries.at(n);
}

size_t Halley::HighScore::getNumEntries() const
{
	return entries.size();
}

size_t Halley::HighScore::getMaxEntries() const
{
	return maxEntries;
}

void Halley::HighScore::addScore(String data, int value)
{
	for (size_t i=0; i<entries.size(); i++) {
		if (value > entries[i].second) {
			// Insert at i
			entries.insert(entries.begin() + i, HighScoreEntry(data, value));
			if (entries.size() > maxEntries) entries.resize(maxEntries);
			break;
		}
	}
}

bool Halley::HighScore::isHighScore(int value) const
{
	if (getNumEntries() < getMaxEntries()) return true;
	return entries[entries.size()-1].second < value;
}

std::vector<char> Halley::HighScore::save() const
{
	return std::vector<char>();
}

void Halley::HighScore::load(std::vector<char>&)
{
	// TODO
}
