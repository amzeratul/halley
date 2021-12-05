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

#include <iostream>
#include "halley/utils/halley_iostream.h"

namespace Halley {
	class Console {
	public:
		enum ColourType {
			NO_CHANGE = -2,
			DEFAULT = -1,
			BLACK,
			DARK_BLUE,
			DARK_GREEN,
			DARK_CYAN,
			DARK_RED,
			DARK_MAGENTA,
			DARK_YELLOW,
			GREY,
			DARK_GREY,
			BLUE,
			GREEN,
			CYAN,
			RED,
			MAGENTA,
			YELLOW,
			WHITE
		};

		static void setForeground(ColourType colour);
		static void setBackground(ColourType colour);
		static Console::ColourType getForeground();
		static Console::ColourType getBackground();

	private:
		static Console::ColourType curForeground;
		static Console::ColourType curBackground;
	};

	class ConsoleColourStack {
	public:
		ConsoleColourStack(Console::ColourType foreground = Console::DEFAULT, Console::ColourType background = Console::NO_CHANGE);
		~ConsoleColourStack();

	private:
		Console::ColourType prevForeground;
		Console::ColourType prevBackground;
	};

	class ConsoleColour {
	public:
		ConsoleColour(Console::ColourType foreground = Console::DEFAULT, Console::ColourType background = Console::NO_CHANGE);

	private:
		Console::ColourType foreground;
		Console::ColourType background;

		friend std::ostream& operator<<(std::ostream& os, const Halley::ConsoleColour& colour);
	};

	inline std::ostream& operator<<(std::ostream& os, const Halley::ConsoleColour& colour)
	{
		if (colour.foreground != Console::NO_CHANGE) Console::setForeground(colour.foreground);
		if (colour.background != Console::NO_CHANGE) Console::setBackground(colour.background);
		return os;
	}

	
}
