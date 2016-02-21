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

namespace Halley {
	class Console {
	public:
		enum ColorType {
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

		static void setForeground(ColorType color);
		static void setBackground(ColorType color);
		static Console::ColorType getForeground();
		static Console::ColorType getBackground();

	private:
		static Console::ColorType curForeground;
		static Console::ColorType curBackground;
	};

	class ConsoleColorStack {
	public:
		ConsoleColorStack(Console::ColorType foreground = Console::DEFAULT, Console::ColorType background = Console::NO_CHANGE);
		~ConsoleColorStack();

	private:
		Console::ColorType prevForeground;
		Console::ColorType prevBackground;
	};

	class ConsoleColor {
	public:
		ConsoleColor(Console::ColorType foreground = Console::DEFAULT, Console::ColorType background = Console::NO_CHANGE);

	private:
		Console::ColorType foreground;
		Console::ColorType background;

		friend std::ostream& operator<<(std::ostream& os, const Halley::ConsoleColor& color);
	};

	inline std::ostream& operator<<(std::ostream& os, const Halley::ConsoleColor& color)
	{
		if (color.foreground != Console::NO_CHANGE) Console::setForeground(color.foreground);
		if (color.background != Console::NO_CHANGE) Console::setBackground(color.background);
		return os;
	}

	
}
