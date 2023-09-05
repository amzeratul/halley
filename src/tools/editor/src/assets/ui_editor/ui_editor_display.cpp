#include "ui_editor_display.h"

#include "ui_editor.h"

using namespace Halley;

UIEditorDisplay::UIEditorDisplay(String id, Vector2f minSize, UISizer sizer, const HalleyAPI& api, Resources& resources)
	: UIWidget(std::move(id), minSize, std::move(sizer))
{
	setCanSendEvents(false);

	RenderSurfaceOptions options;
	options.createDepthStencil = false;
	options.useFiltering = false;

	displayRoot = std::make_shared<UIRenderSurface>("displayRoot", Vector2f(), UISizer(), api, resources, "Halley/Sprite", options);
	displayRoot->setScale(Vector2f(1, 1));
	UIWidget::add(displayRoot, 0, Vector4f(), UISizerAlignFlags::Top | UISizerAlignFlags::Left);
}

void UIEditorDisplay::setUIEditor(UIEditor* uiEditor)
{
	editor = uiEditor;

	if (editor) {
		auto& factory = editor->getGameFactory();
		factory.setConstructionCallback([=](const std::shared_ptr<IUIElement>& element, const String& uuid)
		{
			if (!uuid.isEmpty()) {
				elements[UUID(uuid)] = element;
			}
			const auto widget = std::dynamic_pointer_cast<UIWidget>(element);
			if (widget) {
				maxAdjustment = std::max(maxAdjustment, widget->getChildLayerAdjustment());
			}
		});
	} else {
		curElement = {};
		clearDisplay();
	}
}

void UIEditorDisplay::drawAfterChildren(UIPainter& painter) const
{
	if (curElement) {
		auto p = painter.withAdjustedLayer(maxAdjustment + 1);

		p.draw([&] (Painter& painter)
		{
			painter.drawRect(curRect, 1.0f, Colour4f(0, 1, 0), {}, { 3.0f, 5.0f });
		});
	}
}

void UIEditorDisplay::setZoom(float zoom)
{
	displayRoot->setScale(Vector2f(zoom, zoom));
	doLayout();
}

bool UIEditorDisplay::ignoreClip() const
{
	return true;
}

Rect4f UIEditorDisplay::getCurWidgetRect() const
{
	return curRect - getPosition();
}

void UIEditorDisplay::update(Time time, bool moved)
{
	if (moved) {
		doLayout();
	}
}

void UIEditorDisplay::onLayout()
{
}

void UIEditorDisplay::setSelectedWidget(const String& id)
{
	curSelection = id;
	updateCurWidget();
}

void UIEditorDisplay::clearDisplay()
{
	displayRoot->clear();
	elements.clear();
	maxAdjustment = 0;
}

void UIEditorDisplay::loadDisplay(const UIDefinition& uiDefinition)
{
	clearDisplay();
	editor->getGameFactory().loadUI(*displayRoot, uiDefinition, this);
	updateCurWidget();
}

void UIEditorDisplay::onPlaceInside(Rect4f rect, Rect4f origRect, const std::shared_ptr<IUIElement>& element, UISizer& sizer)
{
	if (rect.getWidth() < 0.0001f) {
		rect = rect.grow(0, 0, 1, 0);
	}
	if (rect.getHeight() < 0.0001f) {
		rect = rect.grow(0, 0, 0, 1);
	}
	if (origRect.getWidth() < 0.0001f) {
		origRect = origRect.grow(0, 0, 1, 0);
	}
	if (origRect.getHeight() < 0.0001f) {
		origRect = origRect.grow(0, 0, 0, 1);
	}

	if (element == curElement) {
		curRect = transformRect(rect);
		curSizer = &sizer;
	}
	sizerRects[&sizer].emplace_back(transformRect(origRect), element == curElement);
}

void UIEditorDisplay::applyTransform(const Matrix4f& matrix)
{
	transform *= matrix;
}

void UIEditorDisplay::updateCurWidget()
{
	curSizer = nullptr;
	curRect = Rect4f();
	curElement = {};
	sizerRects.clear();

	if (!curSelection.isEmpty()) {
		const auto iter = elements.find(UUID(curSelection));
		if (iter != elements.end()) {
			curElement = iter->second;
		}

		doLayout();
	}
}

void UIEditorDisplay::doLayout()
{
	transform = Matrix4f::makeIdentity();
	layout(this);
}

void UIEditorDisplay::onOtherUIReloaded(UIWidget& ui)
{
	updateCurWidget();
}

Rect4f UIEditorDisplay::transformRect(Rect4f r) const
{
	return Rect4f(transform * r.getP1(), transform * r.getP2());
}
