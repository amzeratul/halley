#include "halley/ui/widgets/ui_resize_divider.h"

#include "halley/api/input_api.h"
using namespace Halley;

UIResizeDivider::UIResizeDivider(String id, UIResizeDividerType type)
	: UIWidget(std::move(id), Vector2f(1, 1))
	, type(type)
{
	setInteractWithMouse(true);
}

void UIResizeDivider::update(Time t, bool moved)
{
	if (!gotTarget) {
		acquireTarget();
		loadTargetSize();
		gotTarget = true;
	}
}

void UIResizeDivider::onActiveChanged(bool active)
{
	if (active) {
		loadTargetSize();
	}
}

void UIResizeDivider::onAddedToRoot(UIRoot& root)
{
}

void UIResizeDivider::acquireTarget()
{
	target = {};

	auto* parentWidget = dynamic_cast<UIWidget*>(getParent());
	if (!parentWidget) {
		Logger::logError("UIResizeDivider is not a child of another widget.");
		return;
	}

	auto& parentSizer = parentWidget->tryGetSizer();
	if (!parentSizer) {
		Logger::logError("Parent of UIResizeDivider has no sizer");
		return;
	}

	if (isHorizontal()) {
		if (parentSizer->getType() != UISizerType::Horizontal) {
			Logger::logError("UIResizeDivider expected parent with horizontal sizer (got " + toString(parentSizer->getType()) + ")");
			return;
		}
	} else {
		if (parentSizer->getType() != UISizerType::Vertical) {
			Logger::logError("UIResizeDivider expected parent with vertical sizer (got " + toString(parentSizer->getType()) + ")");
			return;
		}
	}

	const auto& siblings = getParent()->getChildren();

	std::optional<size_t> myIdx;

	const auto n = siblings.size();
	for (size_t i = 0; i < n; ++i) {
		if (siblings[i].get() == this) {
			myIdx = i;
			break;
		}
	}

	if (myIdx) {
		std::optional<size_t> targetIdx;
		if (isTargetBeforeMe()) {
			if (*myIdx > 0) {
				targetIdx = *myIdx - 1;
			}
		} else {
			if (*myIdx < n - 1) {
				targetIdx = *myIdx + 1;
			}
		}

		if (targetIdx) {
			target = siblings[*targetIdx];
		}
	}

	if (!target) {
		Logger::logError("UIResizeDivider could not find widget to target (parent's number of children = " + toString(n) + ", I am " + (myIdx ? toString(*myIdx) : "<undefined>") + ")");
	}
}

bool UIResizeDivider::isHorizontal() const
{
	return type == UIResizeDividerType::HorizontalLeft || type == UIResizeDividerType::HorizontalRight;
}

bool UIResizeDivider::isTargetBeforeMe() const
{
	return type == UIResizeDividerType::HorizontalLeft || type == UIResizeDividerType::VerticalTop;
}

std::optional<MouseCursorMode> UIResizeDivider::getMouseCursorMode() const
{
	return isHorizontal() ? MouseCursorMode::SizeWE : MouseCursorMode::SizeNS;
}

Rect4f UIResizeDivider::getMouseRect() const
{
	return getRect().grow(2);
}

void UIResizeDivider::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		held = true;
		startPos = mousePos;

		if (target) {
			startSize = target->getSize()[isHorizontal() ? 0 : 1];
		}
	}
}

void UIResizeDivider::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		held = false;
	}
}

void UIResizeDivider::onMouseOver(Vector2f mousePos)
{
	if (held && target) {
		setTargetSize(startSize + (mousePos - startPos)[isHorizontal() ? 0 : 1] * (isTargetBeforeMe() ? 1.0f : -1.0f), true);
	}
}

void UIResizeDivider::setTargetSize(float size, bool store)
{
	auto minSize = target->getMinimumSize();
	minSize[isHorizontal() ? 0 : 1] = size;
	target->setMinSize(minSize);

	if (store) {
		getRoot()->setUISetting("resize_divider:" + getId(), ConfigNode(size));
	}
}

void UIResizeDivider::loadTargetSize()
{
	if (target) {
		auto value = getRoot()->getUISetting("resize_divider:" + getId());
		if (value.getType() != ConfigNodeType::Undefined) {
			setTargetSize(value.asFloat(), false);
		}
	}
}
