#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/text/text_renderer.h"
#include <climits>
#include "halley/text/i18n.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class UILabel : public UIWidget {
	public:
		explicit UILabel(const String& id, TextRenderer style, const LocalisedString& text);
		~UILabel();

		void setText(const LocalisedString& text);
		void setFutureText(Future<String> text);
		void setColourOverride(const std::vector<ColourOverride>& overrides);

		void setMaxWidth(float maxWidth);
		void setMaxHeight(float maxHeight);
		float getMaxWidth() const;
		float getMaxHeight() const;
		void setWordWrapped(bool wrapped);
		bool isWordWrapped() const;
		bool isClipped() const;

		void setAlignment(float alignment);

		void setTextRenderer(TextRenderer renderer);
		TextRenderer& getTextRenderer();
		const TextRenderer& getTextRenderer() const;

		void setColour(Colour4f colour);		
		Colour4f getColour() const;

		void setSelectable(TextRenderer normalRenderer, TextRenderer selectedRenderer);
		void setDisablable(TextRenderer normalRenderer, TextRenderer disabledRenderer);
		void setHoverable(TextRenderer normalRenderer, TextRenderer hoveredRenderer);
		
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setMarquee(bool enabled);

	private:
		TextRenderer renderer;
		LocalisedString text;
		const std::shared_ptr<bool> aliveFlag;

		Vector2f textExtents;
		float maxWidth = std::numeric_limits<float>::infinity();
		float maxHeight = std::numeric_limits<float>::infinity();
		bool wordWrapped = true;
		bool needsClip = false;
		bool marquee = false;

		Time marqueeIdle;
		int marqueeDirection = -1;
		float marqueePos = 0;
		float unclippedWidth;

		void updateMinSize();
		void updateText();
		void updateMarquee(Time t);
	};
}
