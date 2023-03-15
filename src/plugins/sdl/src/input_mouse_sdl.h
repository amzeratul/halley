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

#include "halley/input/input_button_base.h"

namespace Halley {

#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif

	class InputMouseSDL final : public InputButtonBase {
		friend class InputSDL;
	public:
		Vector2f getPosition() const override;
		void setPosition(Vector2f position) override;
		int getWheelMove() const override;
		float getAxis(int n) override;
		void clearPresses() override;

		void update();
		InputType getInputType() const override;

	private:
		InputMouseSDL();
		void processEvent(const SDL_Event& event, const std::function<Vector2f(Vector2i)>& remap);
		void updateRemap(const std::function<Vector2f(Vector2i)>& remap);

		// HACK can be removed once we update SDL
		void setDeltaPos(Vector2i deltaPos);
		void setMouseTrapped(bool isTrapped);
		//

		Vector2f pos;
		Vector2f prevPos;
		int wheelMove = 0;
		bool isMouseTrapped = false;
	};

#ifdef _MSC_VER
#pragma warning(default: 4250)
#endif

}
