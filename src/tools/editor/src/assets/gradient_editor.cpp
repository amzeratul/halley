#include "gradient_editor.h"

#include "src/ui/colour_picker.h"

using namespace Halley;

GradientEditorButton::GradientEditorButton(UIFactory& factory, VideoAPI& video, ColourGradient gradient, Callback callback)
	: UIImage(Sprite().setImage(factory.getResources(), "halley_ui/ui_list_item.png", "Halley/SpriteSlicedMask"))
	, factory(factory)
	, video(video)
	, callback(std::move(callback))
	, gradient(std::move(gradient))
{
	setMinSize(Vector2f(40, 22));
	setInteractWithMouse(true);
	updateGradient();
}

void GradientEditorButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<GradientEditorWindow>(factory, gradient, [=] (ColourGradient gradient)
		{
			this->gradient = std::move(gradient);
			updateGradient();
			if (callback) {
				callback(this->gradient);
			}
		}));
	}
}

void GradientEditorButton::updateGradient()
{
	const auto size = Vector2i(128, 1);
	if (!image || image->getSize() != size) {
		image = std::make_shared<Image>(Image::Format::RGBA, size);
	}
	gradient.render(*image);

	gradientImage.setImage(factory.getResources(), video, image);
	getSprite().getMutableMaterial().set(1, gradientImage.getMaterial().getTexture(0));
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

	layout();
	gradientEditor = getWidgetAs<GradientEditor>("gradientEditor");
	gradientEditor->setGradient(gradient);
}

void GradientEditorWindow::accept()
{
	if (callback) {
		gradient = gradientEditor->getGradient();
		callback(gradient);
	}
	destroy();
}

void GradientEditorWindow::cancel()
{
	destroy();
}



GradientEditor::GradientEditor(UIFactory& factory, String id, UIStyle style, VideoAPI& video)
	: UIWidget(std::move(id), Vector2f(), UISizer())
	, factory(factory)
	, video(video)
{
	setInteractWithMouse(true);

	anchorSprite = style.getSprite("anchor");
	anchorColourSprite = style.getSprite("anchorColour");

	updateGradient();
}

void GradientEditor::update(Time t, bool moved)
{
	const auto gradientBox = getGradientBox();
	gradientImage.setPosition(gradientBox.getTopLeft()).setSize(gradientBox.getSize());
}

void GradientEditor::draw(UIPainter& painter) const
{
	const auto gradientBox = getGradientBox();

	painter.draw(gradientImage);

	for (size_t i = 0; i < gradient.positions.size(); ++i) {
		if (i == holdingAnchor && !anchorAlive) {
			continue;
		}

		const auto pos = Vector2f(lerp(gradientBox.getLeft(), gradientBox.getRight(), gradient.positions[i]), gradientBox.getTop());
		auto anchor = anchorSprite.clone().setPosition(pos);
		if (i == currentAnchor) {
			anchor.setColour(anchor.getColour().inverseMultiplyLuma(0.5f));
		}
		const auto anchorColour = anchorColourSprite.clone().setPosition(pos).setColour(gradient.colours[i]);

		painter.draw(anchor, true);
		painter.draw(anchorColour, true);
	}
}

void GradientEditor::setGradient(ColourGradient gradient)
{
	this->gradient = std::move(gradient);
	updateGradient();
}

const ColourGradient& GradientEditor::getGradient() const
{
	return gradient;
}

ColourGradient& GradientEditor::getGradient()
{
	return gradient;
}

void GradientEditor::setChangeCallback(Callback callback)
{
	this->callback = std::move(callback);
}

void GradientEditor::onMouseOver(Vector2f mousePos)
{
	if (holdingAnchor) {
		dragAnchor(*holdingAnchor, mousePos);
	} else {
		currentAnchor = getAnchorUnderMouse(mousePos);
	}
}

void GradientEditor::onMouseLeft(Vector2f mousePos)
{
	currentAnchor = {};
}

void GradientEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		if (currentAnchor) {
			holdingAnchor = currentAnchor;
			anchorAlive = true;
		} else {
			createAnchor(mousePos);
		}
	} else if (button == 2) {
		if (currentAnchor) {
			editAnchor(*currentAnchor);
		}
	}
}

void GradientEditor::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		if (!anchorAlive) {
			deleteAnchor(*holdingAnchor);
		}
		holdingAnchor = {};
	}
}

bool GradientEditor::isFocusLocked() const
{
	return holdingAnchor.has_value();
}

bool GradientEditor::canReceiveFocus() const
{
	return true;
}

Rect4f GradientEditor::getGradientBox() const
{
	return getRect().grow(-6, -24, -6, -6);
}

std::optional<size_t> GradientEditor::getAnchorUnderMouse(Vector2f mousePos) const
{
	const auto gradientBox = getGradientBox();

	float bestDist = std::numeric_limits<float>::infinity();
	std::optional<size_t> bestIdx;

	for (size_t i = 0; i < gradient.positions.size(); ++i) {
		const auto pos = Vector2f(lerp(gradientBox.getLeft(), gradientBox.getRight(), gradient.positions[i]), gradientBox.getTop());
		const auto box = Rect4f(pos.x - 6.0f, pos.y - 24.0f, 12.0f, 24.0f);
		if (box.contains(mousePos)) {
			const float dist = (mousePos - box.getCenter()).length();
			if (dist < bestDist) {
				bestDist = dist;
				bestIdx = i;
			}
		}
	}

	return bestIdx;
}

void GradientEditor::createAnchor(Vector2f mousePos)
{
	const auto gradientBox = getGradientBox();
	const auto pos = clamp((mousePos.x - gradientBox.getLeft()) / gradientBox.getWidth(), 0.0f, 1.0f);

	for (size_t i = 0; i < gradient.positions.size(); ++i) {
		if (pos < gradient.positions[i]) {
			insertAnchorAt(pos, i);
			return;
		}
	}
	insertAnchorAt(pos, gradient.positions.size());
}

void GradientEditor::insertAnchorAt(float pos, size_t idx)
{
	const auto colour = gradient.evaluatePrecomputed(pos);

	gradient.positions.insert(gradient.positions.begin() + idx, pos);
	gradient.colours.insert(gradient.colours.begin() + idx, colour);
	holdingAnchor = idx;

	updateGradient();
}

void GradientEditor::editAnchor(size_t idx)
{
	getRoot()->addChild(std::make_shared<ColourPicker>(factory, gradient.colours[idx].toString(), false, [=] (String col, bool final)
	{
		if (final) {
			gradient.colours[idx] = Colour4f::fromString(col);
			updateGradient();
		}
	}));
}

void GradientEditor::deleteAnchor(size_t idx)
{
	gradient.positions.erase(gradient.positions.begin() + idx);
	gradient.colours.erase(gradient.colours.begin() + idx);
}

void GradientEditor::dragAnchor(size_t idx, Vector2f mousePos)
{
	const auto gradientBox = getGradientBox();
	const auto pos = clamp((mousePos.x - gradientBox.getLeft()) / gradientBox.getWidth(), 0.0f, 1.0f);

	const auto verticalBounds = Range<float>(gradientBox.getTop() - 30.0f, gradientBox.getTop() + 6.0f);
	anchorAlive = verticalBounds.contains(mousePos.y);

	gradient.positions[idx] = pos;

	while (idx > 0 && gradient.positions[idx - 1] > gradient.positions[idx]) {
		std::swap(gradient.positions[idx - 1], gradient.positions[idx]);
		std::swap(gradient.colours[idx - 1], gradient.colours[idx]);
		--idx;
	}
	while (idx < gradient.positions.size() - 1 && gradient.positions[idx] > gradient.positions[idx + 1]) {
		std::swap(gradient.positions[idx], gradient.positions[idx + 1]);
		std::swap(gradient.colours[idx], gradient.colours[idx + 1]);
		++idx;
	}

	holdingAnchor = idx;
	updateGradient();
}

void GradientEditor::updateGradient()
{
	gradientTmp = gradient;
	if (holdingAnchor && !anchorAlive) {
		gradientTmp.positions.erase(gradientTmp.positions.begin() + *holdingAnchor);
		gradientTmp.colours.erase(gradientTmp.colours.begin() + *holdingAnchor);
	}

	const auto rect = getGradientBox();
	const auto size = Vector2i(static_cast<int>(rect.getWidth()), 1);
	if (!image || image->getSize() != size) {
		image = std::make_shared<Image>(Image::Format::RGBA, size);
	}
	gradientTmp.markDirty();
	gradientTmp.render(*image);

	gradientImage.setImage(factory.getResources(), video, image);
}
