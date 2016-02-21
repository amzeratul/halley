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

#include "console.h"
#include "../os/os.h"

using namespace Halley;


void Halley::Console::setForeground(ColorType color)
{
	curForeground = color;
	OS::get().setConsoleColor(curForeground, curBackground);
}

void Halley::Console::setBackground(ColorType color)
{
	curBackground = color;
	OS::get().setConsoleColor(curForeground, curBackground);
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
