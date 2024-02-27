#pragma once
#include "../ui_widget.h"
#include "halley/graphics/text/text_renderer.h"
#include <climits>
#include "halley/text/i18n.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class UILabel : public UIWidget {
	public:
		explicit UILabel(String id, UIStyle style, LocalisedString text);
		explicit UILabel(String id, UIStyle style, TextRenderer renderer, LocalisedString text);
		~UILabel();

		void setText(const LocalisedString& text);
		void setText(LocalisedString&& text);
		void setTextAndColours(LocalisedString text, Vector<ColourOverride> overrides);
		void setTextAndColours(std::pair<LocalisedString, Vector<ColourOverride>> textAndColours);
		void setFutureText(Future<String> text);
		const LocalisedString& getText() const;
		void setColourOverride(Vector<ColourOverride> overrides);

		void setMaxWidth(std::optional<float> maxWidth);
		void setMaxHeight(std::optional<float> maxHeight);
		std::optional<float> getMaxWidth() const;
		std::optional<float> getMaxHeight() const;
		void setWordWrapped(bool wrapped);
		bool isWordWrapped() const;
		bool isClipped() const;
		void setMarquee(std::optional<float> marqueeSpeed);
		void setFlowLayout(bool flow);

		void setAlignment(float alignment);

		void setTextRenderer(TextRenderer renderer);
		TextRenderer& getTextRenderer();
		const TextRenderer& getTextRenderer() const;

		void setColour(Colour4f colour);		
		Colour4f getColour() const;
		
		void setSelectable(TextRenderer normalRenderer, TextRenderer selectedRenderer, bool preserveAlpha = false);
		void setDisablable(TextRenderer normalRenderer, TextRenderer disabledRenderer);
		void setHoverable(TextRenderer normalRenderer, TextRenderer hoveredRenderer);
		
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setFontSize(float size);
		Vector2f getMinimumSize() const override;

	protected:
		void onParentChanged() override;

	private:
		TextRenderer renderer;
		LocalisedString text;
		const std::shared_ptr<bool> aliveFlag;

		Vector2f textExtents;
		Vector2f textMinSize;
		std::optional<float> maxWidth;
		std::optional<float> maxHeight;
		bool wordWrapped = false;
		bool needsClipX = false;
		bool needsClipY = false;
		bool flowLayout = false;

		Time marqueeIdle = 0;
		std::optional<float> marqueeSpeed;
		int marqueeDirection = -1;
		float marqueePos = 0;
		float unclippedWidth;
		float lastCellWidth;

		void updateMinSize();
		void updateText();
		void updateMarquee(Time t);
		float getCellWidth();
	};
}
