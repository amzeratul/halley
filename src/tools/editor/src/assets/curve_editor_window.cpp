#include "curve_editor_window.h"

#include "curve_editor.h"
using namespace Halley;

CurveEditorButton::CurveEditorButton(UIFactory& factory, InterpolationCurve curve, Callback callback)
	: UIWidget("", Vector2f(40, 22), UISizer())
	, factory(factory)
	, callback(std::move(callback))
	, curve(std::move(curve))
{
	setInteractWithMouse(true);
}

void CurveEditorButton::update(Time t, bool moved)
{
	// TODO
}

void CurveEditorButton::draw(UIPainter& painter) const
{
	// TODO
}

void CurveEditorButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<CurveEditorWindow>(factory, curve, callback));
	}
}



CurveEditorWindow::CurveEditorWindow(UIFactory& factory, InterpolationCurve curve, Callback callback)
	: PopupWindow("curveEditor")
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

	getWidgetAs<CurveEditor>("curveEditor");
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
