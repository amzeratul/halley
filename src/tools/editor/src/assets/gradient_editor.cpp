#include "gradient_editor.h"

using namespace Halley;

GradientEditorButton::GradientEditorButton(UIFactory& factory, ColourGradient gradient, Callback callback)
	: UIImage(Sprite().setImage(factory.getResources(), "halley_ui/ui_list_item.png"))
	, factory(factory)
	, callback(std::move(callback))
	, gradient(std::move(gradient))
{
	setMinSize(Vector2f(40, 22));
	setInteractWithMouse(true);
}

void GradientEditorButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<GradientEditorWindow>(factory, gradient, [=] (ColourGradient gradient)
		{
			this->gradient = std::move(gradient);
			if (callback) {
				callback(this->gradient);
			}
			updateGradient();
		}));
	}
}

void GradientEditorButton::updateGradient()
{
	// TODO
}



GradientEditorWindow::GradientEditorWindow(UIFactory& factory, ColourGradient gradient, Callback callback)
	: PopupWindow("gradientEditorWindow")
	, factory(factory)
	, callback(std::move(callback))
	, gradient(std::move(gradient))
{
	factory.loadUI(*this, "halley/gradient_editor");
	setModal(true);
	setAnchor(UIAnchor());
}

void GradientEditorWindow::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
}

void GradientEditorWindow::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool GradientEditorWindow::onKeyPress(KeyboardKeyPress key)
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

void GradientEditorWindow::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		cancel();
	});
}

void GradientEditorWindow::accept()
{
	if (callback) {
		callback(gradient);
	}
	destroy();
}

void GradientEditorWindow::cancel()
{
	destroy();
}
