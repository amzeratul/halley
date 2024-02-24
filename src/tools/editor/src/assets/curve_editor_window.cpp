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

			notifyDataBind(this->curve.toConfigNode());
		}));
	}
}

void CurveEditorButton::readFromDataBind()
{
	if (auto dataBind = getDataBind()) {
		curve = InterpolationCurve(dataBind->getConfigData());
	}
}


void CurveEditorButton::updateLine()
{
	const auto rect = getRect().shrink(4.0f);
	line.clear();
	for (size_t i = 0; i < curve.points.size(); ++i) {
		auto convert = [&](Vector2f p)
		{
			return Vector2f(lerp(rect.getLeft(), rect.getRight(), p.x), lerp(rect.getBottom(), rect.getTop(), p.y));
		};

		const auto tween = curve.tweens[i];
		if (i > 0 && tween != TweenCurve::Linear) {
			const float t0 = curve.points[i - 1].x;
			const float t1 = curve.points[i].x;
			const int nSteps = lroundl((t1 - t0) / 0.01f);
			for (int j = 0; j < nSteps; ++j) {
				const float t = static_cast<float>(j) / static_cast<float>(nSteps);
				const float x = lerp(t0, t1, t);
				line.push_back(convert(Vector2f(x, curve.evaluateRaw(x))));
			}
		}
		line.push_back(convert(curve.points[i]));
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

	auto curveEditor = getWidgetAs<CurveEditor>("curveEditor");
	bindData("baseline", curve.baseline, [=](float value)
	{
		curve.baseline = value;
		curveEditor->setCurve(curve);
	});

	bindData("scale", curve.scale, [=](float value)
	{
		curve.scale = value;
		curveEditor->setCurve(curve);
	});

	curveEditor->setCurve(curve);
	curveEditor->setChangeCallback([=] (InterpolationCurve curve)
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
