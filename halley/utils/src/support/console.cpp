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

#include "halley/support/console.h"

using namespace Halley;


#ifdef _WIN32

#include <comutil.h>
#include <objbase.h>
#include <wincon.h>
#pragma comment(lib, "comsuppw.lib")

static void setConsoleColour(int foreground, int background)
{
	if (foreground == -1) {
		foreground = 7;
	}
	if (background == -1) {
		background = 0;
	}

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, WORD(foreground | (background << 4)));
}

#else

static void setConsoleColor(int, int)
{}

#endif

void Halley::Console::setForeground(ColourType colour)
{
	curForeground = colour;
	setConsoleColour(curForeground, curBackground);
}

void Halley::Console::setBackground(ColourType colour)
{
	curBackground = colour;
	setConsoleColour(curForeground, curBackground);
}

Console::ColourType Halley::Console::getForeground()
{
	return curForeground;
}

Console::ColourType Halley::Console::getBackground()
{
	return curBackground;
}

Console::ColourType Halley::Console::curForeground = Console::GREY;
Console::ColourType Halley::Console::curBackground = Console::BLACK;

Halley::ConsoleColourStack::ConsoleColourStack(Console::ColourType foreground, Console::ColourType background)
{
	prevForeground = Console::getForeground();
	prevBackground = Console::getBackground();
	if (foreground != Console::NO_CHANGE) Console::setForeground(foreground);
	if (background != Console::NO_CHANGE) Console::setBackground(background);
}

Halley::ConsoleColourStack::~ConsoleColourStack()
{
	Console::setForeground(prevForeground);
	Console::setBackground(prevBackground);
}

Halley::ConsoleColour::ConsoleColour(Console::ColourType _foreground, Console::ColourType _background /*= Console::DEFAULT*/)
	: foreground(_foreground)
	, background(_background)
{
}
