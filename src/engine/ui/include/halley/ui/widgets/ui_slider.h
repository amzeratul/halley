#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UISliderBar;

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

	protected:
		void update(Time t, bool moved) override;

	private:
		std::shared_ptr<UISliderBar> sliderBar;

		const float minValue;
		const float maxValue;
		float value;
	};
	
	class UISliderBar : public UIWidget {
	public:
		UISliderBar(UISlider& parent, UIStyle style);
		
		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;
	    void pressMouse(Vector2f mousePos, int button) override;
	    void releaseMouse(Vector2f mousePos, int button) override;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		UISlider& parent;
		bool held = false;

		Sprite bar;
		Sprite barFull;
		Sprite thumb;
	};
}
