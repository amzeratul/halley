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

namespace Halley {
	class InputVibration {
	public:
		virtual ~InputVibration() {}

		virtual bool getState(Time t, float& high, float& low)=0;
	};

	typedef std::shared_ptr<InputVibration> spInputVibration;

	class InputVibrationADSR : public InputVibration {
	public:
		InputVibrationADSR(Time a, Time d, Time s, Time r, float aHigh, float aLow, float dHigh, float dLow);

		bool getState(Time t, float& high, float& low) override;

	private:
		Time a, d, s, r;
		float aHigh;
		float aLow;
		float dHigh;
		float dLow;
		Time at;
	};
}
