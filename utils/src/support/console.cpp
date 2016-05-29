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

static void setConsoleColor(int foreground, int background)
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

void Halley::Console::setForeground(ColorType color)
{
	curForeground = color;
	setConsoleColor(curForeground, curBackground);
}

void Halley::Console::setBackground(ColorType color)
{
	curBackground = color;
	setConsoleColor(curForeground, curBackground);
}

Console::ColorType Halley::Console::getForeground()
{
	return curForeground;
}

Console::ColorType Halley::Console::getBackground()
{
	return curBackground;
}

Console::ColorType Halley::Console::curForeground = Console::GREY;
Console::ColorType Halley::Console::curBackground = Console::BLACK;

Halley::ConsoleColorStack::ConsoleColorStack(Console::ColorType foreground, Console::ColorType background)
{
	prevForeground = Console::getForeground();
	prevBackground = Console::getBackground();
	if (foreground != Console::NO_CHANGE) Console::setForeground(foreground);
	if (background != Console::NO_CHANGE) Console::setBackground(background);
}

Halley::ConsoleColorStack::~ConsoleColorStack()
{
	Console::setForeground(prevForeground);
	Console::setBackground(prevBackground);
}

Halley::ConsoleColor::ConsoleColor(Console::ColorType _foreground, Console::ColorType _background /*= Console::DEFAULT*/)
	: foreground(_foreground)
	, background(_background)
{
}
