#pragma once

#include <SDL3/SDL_events.h>
#include "halley/input/input_button_base.h"

namespace Halley {

	class InputMouseSDL3 final : public InputButtonBase {
		friend class InputSDL3;
	public:
		Vector2f getPosition() const override;
		void setPosition(Vector2f position) override;
		Vector2f getWheelMove() const override;
		Vector2i getWheelMoveDiscrete() const override;
		float getAxis(int n) override;
		void clearPresses() override;

		void update();
		InputType getInputType() const override;
		std::string_view getName() const override;

	private:
		InputMouseSDL3();
		void processEvent(const SDL_Event& event, const std::function<Vector2f(Vector2i)>& remap);
		void updateRemap(const std::function<Vector2f(Vector2i)>& remap);

		// HACK can be removed once we update SDL
		void setDeltaPos(Vector2i deltaPos);
		void setMouseTrapped(bool isTrapped);
		//

		Vector2f pos;
		Vector2f prevPos;
		Vector2f relMove;
		Vector2f wheelMove;
		Vector2i wheelMoveDiscrete;
		bool isMouseTrapped = false;
	};

}
