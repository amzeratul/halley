#pragma once

#include "halley/maths/vector2.h"
#include "halley/maths/vector4.h"
#include "halley/maths/rect.h"
#include "ui_element.h"

namespace Halley {
	enum class UISizerType {
		Undefined,
		Horizontal,
		Vertical,
		Grid,
		Free
	};

	template <>
	struct EnumNames<UISizerType> {
		constexpr std::array<const char*, 5> operator()() const {
			return {{
				"undefined",
				"horizontal",
				"vertical",
				"grid",
				"free"
			}};
		}
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
		UISizerEntry(UIElementPtr widget, float proportion, Vector4f border, int fillFlags, Vector2f position);
	
		float getProportion() const;
		Vector2f getMinimumSize() const;
		Vector4f getBorder() const;
		int getFillFlags() const;
		Vector2f getPosition() const;

		void placeInside(Rect4f rect, Vector2f minSize);

		UIElementPtr getPointer() const;

		bool isEnabled() const;
		void updateEnabled() const;

		void setBorder(const Vector4f& border);

		void setProportion(float prop);
		void setPosition(Vector2f pos);

	private:
		UIElementPtr widget;
		float proportion;
		Vector4f border;
		int fillFlags;
		Vector2f position;
		mutable bool enabled = true;
	};

	class UIWidget;
	class UISizer;
	class UIParent;

	class IUISizer {
	public:
		virtual ~IUISizer() {}

		virtual void add(std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill, Vector2f position = Vector2f()) = 0;
		virtual void addSpacer(float size) = 0;
		virtual void addStretchSpacer(float proportion = 0) = 0;
		virtual void remove(IUIElement& element) = 0;
	};

	class UISizer final : public IUIElement, public IUISizer {
	public:
		explicit UISizer(UISizerType type = UISizerType::Horizontal, float gap = 1.0f, int nColumns = 0);

		UISizer(UISizer&& other) noexcept;
		UISizer(const UISizer& other) = delete;
		UISizer& operator=(UISizer&& other) noexcept;
		UISizer& operator=(const UISizer& other) = delete;

		Vector2f getLayoutMinimumSize(bool force) const override;
		void setRect(Rect4f rect) override;

		void add(std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill, Vector2f position = Vector2f()) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;
		void remove(IUIElement& element) override;

		void reparent(UIParent& parent);

		void clear();
		bool isActive() const override;

		void setColumnProportions(const std::vector<float>& values);
		void setRowProportions(const std::vector<float>& values);
		void setEvenColumns();

		UISizerType getType() const;
		size_t size() const;
		const UISizerEntry& operator[](size_t n) const;
		UISizerEntry& operator[](size_t n);

		void updateEnabled() const;
		
		void swapItems(int idxA, int idxB);

		template <typename F>
		void sortItems(F f)
		{
			std::sort(entries.begin(), entries.end(), f);
		}

	private:
		std::vector<UISizerEntry> entries;
		std::vector<float> columnProportions;
		std::vector<float> rowProportions;

		UISizerType type = UISizerType::Undefined;
		float gap = 1.0f;
		int nColumns = false;

		UIParent* curParent = nullptr;

		void reparentEntry(UISizerEntry& entry);
		void unparentEntry(UISizerEntry& entry);

		Vector2f computeMinimumSize(bool includeProportional) const;

		Vector2f computeMinimumSizeBox(bool includeProportional) const;
		void setRectBox(Rect4f rect);

		Vector2f computeMinimumSizeBoxFree() const;
		void setRectBoxFree(Rect4f rect);

		void computeGridSizes(std::vector<float>& cols, std::vector<float>& rows) const;
		Vector2f computeMinimumSizeGrid() const;
		void setRectGrid(Rect4f rect);
		float getColumnProportion(int column) const;
		float getRowProportion(int row) const;
	};
}
