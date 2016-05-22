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

#include "input_button_base.h"


namespace Halley {
	class InputManual : public InputButtonBase {
		class Command {
		public:
			int button;
			int type;
		};

	public:
		InputManual(int nButtons, int nAxes);

		void update();
		void pressButton(int n);
		void holdButton(int n);
		void releaseButton(int n);
		void setAxis(int n, float value);

		size_t getNumberAxes() override;
		float getAxis(int n) override;

	private:
		std::vector<Command> commands;
		std::vector<float> axes;
	};

	typedef std::shared_ptr<InputManual> spInputManual;
}
