#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UISliderBar;
	class UILabel;

	class UISlider : public UIWidget {
	public:
		UISlider(const String& id, UIStyle style, float minValue, float maxValue, float value = 0);

		void setValue(float value);
		void setRelativeValue(float value);
		float getValue() const;
		float getMinValue() const;
		float getMaxValue() const;
		float getRelativeValue() const; // 0..1 range
		void readFromDataBind() override;

		void onManualControlAnalogueAdjustValue(float delta, Time t) override;

	protected:
		void update(Time t, bool moved) override;

	private:
		std::shared_ptr<UISliderBar> sliderBar;
		std::shared_ptr<UILabel> label;

		const float minValue;
		const float maxValue;
		float value;
		float maxSpeed = 0;
		Time timeSinceMove = 0;
	};
	
	class UISliderBar : public UIWidget {
	public:
		UISliderBar(UISlider& parent, UIStyle style);
		
		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;
	    void pressMouse(Vector2f mousePos, int button) override;
	    void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		Rect4f getMouseRect() const override;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		UISlider& parent;
		bool held = false;
		Vector4f extra;

		Sprite bar;
		Sprite barFull;
		Sprite thumb;
		Sprite left;
		Sprite right;

		void fill(UIPainter& painter, Rect4f rect, Sprite sprite) const;
	};
}
