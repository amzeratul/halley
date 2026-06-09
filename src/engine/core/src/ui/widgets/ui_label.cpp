#include "halley/ui/widgets/ui_label.h"
#include "halley/text/i18n.h"
#include "halley/text/string_output_server.h"

using namespace Halley;

UILabel::UILabel(String id, UIStyle style, LocalisedString text)
	: UIWidget(std::move(id), {})
	, renderer(style.getTextRenderer("label"))
	, text(std::move(text))
	, aliveFlag(std::make_shared<bool>(true))
{
	styles.emplace_back(std::move(style));
	updateText();
}

UILabel::UILabel(String id, UIStyle style, TextRenderer renderer, LocalisedString text)
	: UIWidget(std::move(id), {})
	, renderer(std::move(renderer))
	, text(std::move(text))
	, aliveFlag(std::make_shared<bool>(true))
{
	styles.emplace_back(std::move(style));
	updateText();
}

UILabel::~UILabel()
{
	*aliveFlag = false;
}

void UILabel::draw(UIPainter& painter) const
{
	if (needsClipX || needsClipY || worldClip) {
		auto rect = getRect();
		if (worldClip) {
			rect = rect.intersection(*worldClip);
		} else {
			if (!needsClipX) {
				rect = rect.grow(50, 0, 50, 0);
			}
			if (!needsClipY) {
				rect = rect.grow(0, 50, 0, 50);
			}
		}
		painter.withClip(rect).draw(renderer);
	} else {
		painter.draw(renderer);
	}

	if (renderer.getColour().a > 0) {
		if (auto* stringOutputServer = getRoot()->tryGetStringOutputServer()) {
			uint64_t id = reinterpret_cast<uint64_t>(this) / 16;
			stringOutputServer->reportString(toString(id), StringOutputType::Generic, text, {}); // TODO: collect this data properly
		}
	}
}

void UILabel::update(Time t, bool moved)
{
	if (flowLayout && lastCellWidth != getCellWidth()) {
		updateText();
	}
	if (marqueeSpeed) {
		updateMarquee(t);
	}
	if (text.checkForUpdates()) {
		updateText(false);
	}
	if (moved || marqueeSpeed) {
		renderer.setPosition(getPosition() + Vector2f(renderer.getAlignment() * textExtents.x - marqueePos, 0.0f) - textBounds.getTopLeft());
	}
}

void UILabel::setFont(std::shared_ptr<const Font> font)
{
	renderer.setFont(std::move(font));
	needsMinSize = true;
}

void UILabel::setFontSize(float size)
{
	renderer.setSize(size);
	needsMinSize = true;
}

void UILabel::setMarquee(std::optional<float> speed)
{
	marqueeSpeed = speed;
	if (marqueeSpeed) {
		wordWrapped = false;
	} else {
		marqueePos = 0;
		marqueeIdle = 0;
		marqueeDirection = -1;
	}
}

void UILabel::updateMinSize()
{
	needsMinSize = false;

	lastCellWidth = getCellWidth();
	const float effectiveMaxWidth = std::min(lastCellWidth, maxWidth.value_or(std::numeric_limits<float>::max()));

	needsClipX = needsClipY = false;
	auto curExtents = renderer.getExtents();
	unclippedWidth = curExtents.x;
	if (curExtents.x > effectiveMaxWidth) {
		if (wordWrapped || flowLayout) {
			renderer.setText(renderer.split(effectiveMaxWidth));
			curExtents = renderer.getExtents();
			unclippedWidth = curExtents.x;
			needsClipX = curExtents.x > effectiveMaxWidth;
		} else {
			unclippedWidth = curExtents.x;
			curExtents.x = effectiveMaxWidth;
			needsClipX = true;
		}
	}
	if (curExtents.y > maxHeight.value_or(std::numeric_limits<float>::max())) {
		float maxLines = std::floor(maxHeight.value_or(std::numeric_limits<float>::max()) / renderer.getLineHeight());
		curExtents.y = maxLines * renderer.getLineHeight();
		needsClipY = true;
	}

	auto textMinSize = (flowLayout ? Vector2f(0.0f, curExtents.y) : curExtents).ceil();
	if (testMaxWidth && maxWidth) {
		textMinSize.x = std::max(textMinSize.x, *maxWidth);
	}

	const auto oldTextBounds = textBounds;
	textBounds = Rect4f(Vector2f(), textMinSize).rotate(renderer.getAngle());
	textExtents = curExtents;

	if (textBounds != oldTextBounds) {
		markAsNeedingLayout();
	}
}

void UILabel::updateText(bool allowReplay) {
	renderer.setText(text);
	needsMinSize = true;
	if (allowReplay && replayOnModified) {
		replayInitialBehaviours();
	}
}

void UILabel::updateMarquee(Time t)
{
	if (needsClipX) {
		if (marqueeIdle > 0) {
			marqueeIdle -= t;
			return;
		}
		const float speed = *marqueeSpeed;
		const float maxMarquee = unclippedWidth - maxWidth.value_or(std::numeric_limits<float>::max());
		marqueePos += float(marqueeDirection) * float(t) * speed;
		if (marqueePos < 0 || marqueePos > maxMarquee) {
			marqueePos = clamp(marqueePos, 0.0f, maxMarquee);
			marqueeDirection = -marqueeDirection;
			marqueeIdle = 0.5;
		}
	} else {
		marqueePos = 0;
		marqueeIdle = 0;
		marqueeDirection = -1;
	}
}

float UILabel::getCellWidth()
{
	if (flowLayout) {
		auto parent = getParent();
		if (parent) {
			auto max = parent->getMaxChildWidth();
			if (max) {
				return max.value();
			}
		}
	}
	
	return std::numeric_limits<float>::max();
}

void UILabel::setText(const LocalisedString& t)
{
	if (text != t) {
		setText(LocalisedString(t));
	}
}

void UILabel::setText(LocalisedString&& t)
{
	t.checkForUpdates();
	if (text != t) {
		const bool isEquivalent = text.isSameKeyAndTransform(t);
		text = std::move(t);
		updateText(!isEquivalent);
	}
}

void UILabel::setTextAndColours(LocalisedString text, Vector<ColourOverride> overrides)
{
	setText(std::move(text));
	setColourOverride(std::move(overrides));
}

void UILabel::setTextAndColours(std::pair<LocalisedString, Vector<ColourOverride>> textAndColours)
{
	setText(std::move(textAndColours.first));
	setColourOverride(std::move(textAndColours.second));
}

void UILabel::setFutureText(Future<String> futureText)
{
	const auto flag = aliveFlag;
	futureText.then(Executors::getMainUpdateThread(), [=, this] (const String& filtered)
	{
		if (*flag) {
			setText(LocalisedString::fromUserString(filtered));
		}
	});
}

const LocalisedString& UILabel::getText() const
{
	return text;
}

void UILabel::setColourOverride(Vector<ColourOverride> overrides)
{
	renderer.setColourOverride(std::move(overrides));
}

void UILabel::setMaxWidth(std::optional<float> m)
{
	if (maxWidth != m) {
		maxWidth = m;
		needsMinSize = true;
		updateText();
	}
}

void UILabel::setMaxHeight(std::optional<float> m)
{
	if (maxHeight != m) {
		maxHeight = m;
		needsMinSize = true;
		updateText();
	}
}

std::optional<float> UILabel::getMaxWidth() const
{
	return maxWidth;
}

std::optional<float> UILabel::getMaxHeight() const
{
	return maxHeight;
}

void UILabel::setWordWrapped(bool wrapped)
{
	if (wordWrapped != wrapped) {
		wordWrapped = wrapped;
		needsMinSize = true;
		updateText();
	}
}

bool UILabel::isWordWrapped() const
{
	return wordWrapped;
}

bool UILabel::isClipped() const
{
	return needsClipX || needsClipY;
}

void UILabel::setFlowLayout(bool flow)
{
	flowLayout = flow;
	updateText();
}

void UILabel::setAlignment(float alignment)
{
	renderer.setAlignment(alignment);
}

TextRenderer& UILabel::getTextRenderer()
{
	return renderer;
}

const TextRenderer& UILabel::getTextRenderer() const
{
	return renderer;
}

Colour4f UILabel::getColour() const
{
	return renderer.getColour();
}

void UILabel::setTextRenderer(TextRenderer r)
{
	r.setText(text).setPosition(renderer.getPosition());
	renderer = std::move(r);
	needsMinSize = true;
}

void UILabel::setColour(Colour4f colour)
{
	renderer.setColour(colour);
}

void UILabel::setSelectable(TextRenderer normalRenderer, TextRenderer selectedRenderer, bool preserveAlpha)
{
	setHandle(UIEventType::SetSelected, [=, this] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			auto col = selectedRenderer.getColour();
			if (preserveAlpha) {
				col.a = getColour().a;
			}

			setColour(col);
			renderer.setOutline(selectedRenderer.getOutline());
			renderer.setOutlineColour(selectedRenderer.getOutlineColour());
		} else {
			auto col = normalRenderer.getColour();
			if (preserveAlpha) {
				col.a = getColour().a;
			}

			setColour(col);
			renderer.setOutline(normalRenderer.getOutline());
			renderer.setOutlineColour(normalRenderer.getOutlineColour());
		}
	});
}

/*
void UILabel::setSelectable(TextRenderer selectedRenderer)
{
	Colour4f origCol = getColour();
	Colour4f origOutlineCol = renderer.getOutlineColour();
	float origOutline = renderer.getOutline();

	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event) mutable
	{
		if (event.getBoolData()) {
			origCol = getColour();
			origOutlineCol = renderer.getOutlineColour();
			origOutline = renderer.getOutline();

			setColour(selectedRenderer.getColour());
			renderer.setOutline(selectedRenderer.getOutline());
			renderer.setOutlineColour(selectedRenderer.getOutlineColour());
		} else {
			setColour(origCol);
			renderer.setOutline(origOutline);
			renderer.setOutlineColour(origOutlineCol);
		}
	});
}
*/

void UILabel::setDisablable(TextRenderer normalRenderer, TextRenderer disabledRenderer)
{
	setHandle(UIEventType::SetEnabled, [=, this] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			setColour(normalRenderer.getColour());
			renderer.setOutline(normalRenderer.getOutline());
			renderer.setOutlineColour(normalRenderer.getOutlineColour());
		}
		else {
			setColour(disabledRenderer.getColour());
			renderer.setOutline(disabledRenderer.getOutline());
			renderer.setOutlineColour(disabledRenderer.getOutlineColour());
		}
	});
}

void UILabel::setHoverable(TextRenderer normalRenderer, TextRenderer hoveredRenderer)
{
	setHandle(UIEventType::SetHovered, [=, this](const UIEvent& event)
	{
		if (event.getBoolData()) {
			setColour(hoveredRenderer.getColour());
			renderer.setOutline(hoveredRenderer.getOutline());
			renderer.setOutlineColour(hoveredRenderer.getOutlineColour());
		}
		else {
			setColour(normalRenderer.getColour());
			renderer.setOutline(normalRenderer.getOutline());
			renderer.setOutlineColour(normalRenderer.getOutlineColour());
		}
	});
}

void UILabel::onParentChanged()
{
	if (flowLayout) {
		updateText();
	}
}

Vector2f UILabel::getMinimumSize() const
{
	if (needsMinSize) {
		const_cast<UILabel*>(this)->updateMinSize();
	}
	return Vector2f::max(textBounds.getSize(), UIWidget::getMinimumSize());
}

void UILabel::setAngle(Angle1f angle)
{
	renderer.setAngle(angle);
	needsMinSize = true;
}

void UILabel::setLineSpacing(float spacing)
{
	renderer.setLineSpacing(spacing);
	needsMinSize = true;
}

void UILabel::setDynamicValue(std::string_view key, ConfigNode value)
{
	if (key == "alpha") {
		setColour(getColour().withAlpha(value.asFloat(1.0f)));
	}
}

void UILabel::setReplayBehavioursOnModified(bool replayOnModified)
{
	this->replayOnModified = replayOnModified;
}

void UILabel::setWorldClip(Rect4f rect)
{
	worldClip = rect;
}

void UILabel::setTestMaxWidth(bool test)
{
	testMaxWidth = test;
	needsMinSize = true;
}

bool UILabel::isTestingMaxWidth() const
{
	return testMaxWidth;
}
