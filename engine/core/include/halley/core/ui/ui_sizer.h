#pragma once

#include "halley/maths/vector2.h"
#include "halley/maths/vector4.h"
#include "halley/maths/rect.h"

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

	class IUISizeable {
	public:
		virtual ~IUISizeable() {}

		virtual Vector2f computeMinimumSize() const = 0;
		virtual void setRect(Rect4f rect) = 0;
	};

	using UISizeablePtr = std::shared_ptr<IUISizeable>;

	class UISizerEntry {
	public:
		UISizerEntry();
		UISizerEntry(UISizeablePtr widget, float proportion, Vector4f border, int fillFlags);
	
		float getProportion() const;
		Vector2f getMinimumSize() const;
		Vector4f getBorder() const;
		int getFillFlags() const;

		void placeInside(Rect4f rect, Vector2f minSize);
	
	private:
		UISizeablePtr widget;
		float proportion;
		Vector4f border;
		int fillFlags;
	};

	class UISizer : public IUISizeable {
	public:
		explicit UISizer(UISizerType type = UISizerType::Horizontal, float gap = 1.0f);

		Vector2f computeMinimumSize() const override;
		void setRect(Rect4f rect) override;

		void add(UISizeablePtr widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill);

		UISizerType getType() const;
		size_t size() const;
		const UISizerEntry& operator[](size_t n) const;
		UISizerEntry& operator[](size_t n);

	private:
		std::vector<UISizerEntry> entries;
		UISizerType type = UISizerType::Undefined;
		float gap;

		Vector2f computeMinimumSize(bool includeProportional) const;
	};
}
