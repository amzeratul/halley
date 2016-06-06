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

#include <halley/utils/utils.h>
#include "input/input_vibration.h"

using namespace Halley;

Halley::InputVibrationADSR::InputVibrationADSR(Time _a, Time _d, Time _s, Time _r, float _aHigh, float _aLow, float _dHigh, float _dLow)
	: a(_a)
	, d(_d)
	, s(_s)
	, r(_r)
	, aHigh(_aHigh)
	, aLow(_aLow)
	, dHigh(_dHigh)
	, dLow(_dLow)
	, at(0)
{
}

bool Halley::InputVibrationADSR::getState(Time t, float& high, float& low)
{
	at += t;

	if (at < a) {
		// Attack
		float v = static_cast<float>(at) / static_cast<float>(a);
		high = interpolate(0.0f, aHigh, v);
		low = interpolate(0.0f, aLow, v);
	}
	else if (at < a+d) {
		// Decay
		float v = static_cast<float>(at-a) / static_cast<float>(d);
		high = interpolate(aHigh, dHigh, v);
		low = interpolate(aLow, dLow, v);
	}
	else if (at < a+d+s) {
		// Sustain
		high = dHigh;
		low = dLow;
	}
	else {
		// Release
		float v = static_cast<float>(at-a-d-s) / static_cast<float>(r);
		high = interpolate(dHigh, 0.0f, v);
		low = interpolate(dLow, 0.0f, v);
	}

	return at < a+d+s+r;
}
