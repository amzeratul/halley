#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/animation_player.h"

namespace Halley
{
	class AnimationPlayer;

	class UIAnimation : public UIWidget
	{
	public:
		UIAnimation(const String& id, Vector2f size, Vector2f animationOffset, AnimationPlayer animation);

		AnimationPlayer& getPlayer();
		const AnimationPlayer& getPlayer() const;
		Sprite& getSprite();
		const Sprite& getSprite() const;

		Vector2f getOffset() const;
		void setOffset(Vector2f offset);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		Vector2f offset;
		AnimationPlayer animation;
		Sprite sprite;
	};
}
