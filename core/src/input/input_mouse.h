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

#include "input_button_base.h"

namespace Halley {

	class InputMouse : public virtual InputDevice {
	public:
		virtual ~InputMouse() {}
		virtual Vector2i getPosition() const=0;
		virtual int getWheelMove() const=0;
	};

#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif
	class InputMouseConcrete : public InputButtonBase, public InputMouse {
		friend class Input;
	public:
		Vector2i getPosition() const override;
		int getWheelMove() const override;

		void update();

	private:
		InputMouseConcrete();
		void processEvent(const SDL_Event& event);

		Vector2i pos;
		int wheelMove;
	};
#ifdef _MSC_VER
#pragma warning(default: 4250)
#endif

	typedef std::shared_ptr<InputMouse> spInputMouse;
	typedef std::shared_ptr<InputMouseConcrete> spInputMouseConcrete;
}
