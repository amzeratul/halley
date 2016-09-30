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

  Copyright (c) 2007-2012 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "halley/core/input/input_mouse.h"

namespace Halley {

#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif

	class InputMouseSDL : public InputButtonBase, public InputMouse {
		friend class InputSDL;
	public:
		Vector2f getPosition() const override;
		int getWheelMove() const override;

		void update();

	private:
		InputMouseSDL();
		void processEvent(const SDL_Event& event, std::function<Vector2f(Vector2i)> remap);

		Vector2f pos;
		int wheelMove;
	};

#ifdef _MSC_VER
#pragma warning(default: 4250)
#endif

}
