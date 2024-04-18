#pragma once
#include "halley/ui/ui_widget.h"
#include "ui_image.h"
#include "ui_spin_control2.h"

namespace Halley {
	class UISliderBar;
	class UILabel;

	class UISlider : public UIWidget {
	public:
		UISlider(const String& id, UIStyle style, float minValue, float maxValue, float value = 0, bool hasSpinControl = false, bool allowFloat = false);

		void setValue(float value);
		void setRelativeValue(float value);
		float getValue(bool applyGranularity = true) const;
		float getMinValue() const;
		float getMaxValue() const;
		float getRelativeValue(bool applyGranularity = true) const; // 0..1 range
		void readFromDataBind() override;
		void setGranularity(std::optional<float> granularity);
		std::optional<float> getGranularity() const;
		void setMouseWheelSpeed(float speed);

		void setShowLabel(bool show);
		void setLabelConversion(std::function<LocalisedString(float)> f);
		void setTransformation(std::function<float(float)> f);

		void onManualControlAnalogueAdjustValue(float delta, Time t) override;

		std::shared_ptr<UIWidget> getLabelBox() const;

	protected:
		bool canInteractWithMouse() const override;
		void update(Time t, bool moved) override;

	private:
		std::shared_ptr<UISliderBar> sliderBar;
		std::shared_ptr<UILabel> label;
		std::shared_ptr<UISpinControl2> spinControl;
		std::shared_ptr<UIImage> box;

		const float minValue;
		const float maxValue;
		std::optional<float> granularity;
		float mouseWheelSpeed = 1;
		float value;
		float maxSpeed = 0;
		Time timeSinceMove = 0;
		bool fromInput = false;

		std::function<LocalisedString(float)> labelConversion;
		std::function<float(float)> transformation;

		LocalisedString makeLabel(bool forceMax) const;
		void updateLabel();
		float getValueWithGranularity() const;
	};
	
	class UISliderBar : public UIWidget {
	public:
		UISliderBar(UISlider& parent, UIStyle style);
		
		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;
	    void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
	    void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		Rect4f getMouseRect() const override;
		LocalisedString getToolTip() const override;

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
