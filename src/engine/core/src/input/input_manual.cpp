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

#include "input/input_manual.h"


Halley::InputManual::InputManual(int nButtons, int nAxes)
	: InputButtonBase(nButtons)
{
	axes.resize(nAxes);
}

void Halley::InputManual::update()
{
	clearPresses();
	for (size_t i=0; i<commands.size(); i++) {
		Command& c = commands[i];
		if (c.type == 0) {
			onButtonPressed(c.button);
			onButtonReleased(c.button);
		} else if (c.type == 1) {
			onButtonPressed(c.button);
		} else if (c.type == 2) {
			onButtonReleased(c.button);
		}
	}
	commands.clear();
}

void Halley::InputManual::pressButton(int n)
{
	commands.push_back(Command());
	commands.back().button = n;
	commands.back().type = 0;
}

void Halley::InputManual::holdButton(int n)
{
	commands.push_back(Command());
	commands.back().button = n;
	commands.back().type = 1;
}

void Halley::InputManual::releaseButton(int n)
{
	commands.push_back(Command());
	commands.back().button = n;
	commands.back().type = 2;
}

void Halley::InputManual::setAxis(int n, float value)
{
	axes.at(n) = value;
}

size_t Halley::InputManual::getNumberAxes()
{
	return axes.size();
}

float Halley::InputManual::getAxis(int n)
{
	return axes.at(n);
}
