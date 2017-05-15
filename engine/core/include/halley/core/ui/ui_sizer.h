#pragma once

#include "halley/maths/vector2.h"
#include "halley/maths/vector4.h"
#include "halley/maths/rect.h"
#include "ui_element.h"

namespace Halley {
	enum class UISizerType {
		Undefined,
		Horizontal,
		Vertical
	};

	namespace UISizerFillFlags {
		enum Type {
			FillHorizontal = 0x1,
			FillVertical = 0x2,
			Fill = FillHorizontal | FillVertical
		};
	}

	namespace UISizerAlignFlags {
		enum Type {
			Left = 0x4,
			Right = 0x8,
			Top = 0x10,
			Bottom = 0x20,
			CentreHorizontal = 0x40,
			CentreVertical = 0x80,

			Centre = CentreHorizontal | CentreVertical
		};
	}

	class UISizerEntry {
	public:
		UISizerEntry();
		UISizerEntry(UIElementPtr widget, float proportion, Vector4f border, int fillFlags);
	
		float getProportion() const;
		Vector2f getMinimumSize() const;
		Vector4f getBorder() const;
		int getFillFlags() const;

		void placeInside(Rect4f rect, Vector2f minSize);

		UIElementPtr getPointer() const;
		bool isEnabled() const;

	private:
		UIElementPtr widget;
		float proportion;
		Vector4f border;
		int fillFlags;
	};

	class UIWidget;
	class UISizer;
	class UIParent;

	class IUISizer {
	public:
		virtual ~IUISizer() {}

		virtual void add(std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) = 0;
		virtual void add(std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) = 0;
		virtual void addSpacer(float size) = 0;
		virtual void addStretchSpacer(float proportion = 0) = 0;
	};

	class UISizer : public IUIElement, public IUISizer {
	public:
		explicit UISizer(UISizerType type = UISizerType::Horizontal, float gap = 1.0f);

		Vector2f computeMinimumSize() const override;
		void setRect(Rect4f rect) override;

		void add(std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void add(std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;

		void reparent(UIParent& parent);

		void clear();
		bool isShown() const override;

		UISizerType getType() const;
		size_t size() const;
		const UISizerEntry& operator[](size_t n) const;
		UISizerEntry& operator[](size_t n);

	private:
		std::vector<UISizerEntry> entries;
		UISizerType type = UISizerType::Undefined;
		float gap;

		Vector2f computeMinimumSize(bool includeProportional) const;
		void addElement(UIElementPtr widget, float proportion, Vector4f border, int fillFlags);
	};
}
