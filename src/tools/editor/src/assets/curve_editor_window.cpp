#include "curve_editor_window.h"

#include "curve_editor.h"
using namespace Halley;

CurveEditorButton::CurveEditorButton(UIFactory& factory, InterpolationCurve curve, Callback callback)
	: UIImage(Sprite().setImage(factory.getResources(), "halley_ui/ui_list_item.png"))
	, factory(factory)
	, callback(std::move(callback))
	, curve(std::move(curve))
{
	setMinSize(Vector2f(40, 22));

	const auto style = factory.getStyle("curveEditor");
	getSprite().setColour(style.getSprite("display").getColour());
	lineColour = style.getColour("lineColour");

	setInteractWithMouse(true);
}

void CurveEditorButton::update(Time t, bool moved)
{
	UIImage::update(t, moved);

	if (moved) {
		updateLine();
	}
}

void CurveEditorButton::draw(UIPainter& painter) const
{
	UIImage::draw(painter);
	painter.draw([=] (Painter& painter)
	{
		painter.drawLine(line, 1.0f, lineColour);
	});
}

void CurveEditorButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<CurveEditorWindow>(factory, curve, [=] (InterpolationCurve curve)
		{
			this->curve = std::move(curve);
			if (callback) {
				callback(this->curve);
			}
			updateLine();
		}));
	}
}


void CurveEditorButton::updateLine()
{
	const auto rect = getRect().shrink(4.0f);
	line.clear();
	for (const auto& p: curve.points) {
		const auto p2 = Vector2f(lerp(rect.getLeft(), rect.getRight(), p.x), lerp(rect.getBottom(), rect.getTop(), p.y));
		line.push_back(p2);
	}
}

CurveEditorWindow::CurveEditorWindow(UIFactory& factory, InterpolationCurve curve, Callback callback)
	: PopupWindow("curveEditorWindow")
	, factory(factory)
	, callback(std::move(callback))
	, curve(std::move(curve))
{
	factory.loadUI(*this, "halley/curve_editor");
	setModal(true);
	setAnchor(UIAnchor());
}

void CurveEditorWindow::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
}

void CurveEditorWindow::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void CurveEditorWindow::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		cancel();
	});

	bindData("scale", curve.scale, [=](float value)
	{
		curve.scale = value;
	});

	getWidgetAs<CurveEditor>("curveEditor")->setCurve(curve);
	getWidgetAs<CurveEditor>("curveEditor")->setChangeCallback([=] (InterpolationCurve curve)
	{
		this->curve = std::move(curve);
	});
}

bool CurveEditorWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		accept();
		return true;
	}

	if (key.is(KeyCode::Esc)) {
		cancel();
		return true;
	}

	return false;
}

void CurveEditorWindow::update(Time t, bool moved)
{
}

void CurveEditorWindow::accept()
{
	if (callback) {
		callback(curve);
	}
	destroy();
}

void CurveEditorWindow::cancel()
{
	destroy();
}
