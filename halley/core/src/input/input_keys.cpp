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

#include <SDL.h>
#include "input/input_keys.h"

Halley::String Halley::Keys::getName(Key key)
{
	if (!init) {
		doInit();
	}

	auto res = keyNames.find(key);
	if (res != keyNames.end()) return res->second;

	String result = SDL_GetKeyName(SDL_Keycode(key));
	return result;
}

void Halley::Keys::doInit()
{
	keyNames[Esc] = "Esc";
	keyNames[Delete] = "Del";
	for (int i=A; i<=Z; i++) {
		keyNames[Key(i)] = String((i-A)+'A');
	}
	init = true;
}

bool Halley::Keys::init = false;

std::map<Halley::Keys::Key, Halley::String> Halley::Keys::keyNames;
