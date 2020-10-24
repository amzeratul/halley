#include "animation_editor.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/tools/project/project.h"
#include "halley/ui/widgets/ui_animation.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "src/ui/scroll_background.h"

using namespace Halley;

AnimationEditor::AnimationEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project)
	: AssetEditor(factory, resources, project, type)
{
	setupWindow();
}

void AnimationEditor::reload()
{
	loadAssetData();
}

void AnimationEditor::update(Time t, bool moved)
{
	const auto mousePos = Vector2i(animationDisplay->getMousePos());
	const auto size = animationDisplay->getBounds().getSize();
	String str = String("x: ") + toString(mousePos.x) + " y: " + toString(mousePos.y) + " (" + toString(size.x) + "x" + toString(size.y) + ")";
	info->setText(LocalisedString::fromUserString(str));
}

void AnimationEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/animation_editor"), 1);
	animationDisplay = getWidgetAs<AnimationEditorDisplay>("display");
	info = getWidgetAs<UILabel>("info");
	scrollBg = getWidgetAs<ScrollBackground>("scrollBackground");

	scrollBg->setZoomListener([=] (float zoom)
	{
		animationDisplay->setZoom(zoom);
	});

	scrollBg->setMousePosListener([=] (Vector2f mousePos)
	{
		animationDisplay->onMouseOver(mousePos);
	});

	setHandle(UIEventType::DropboxSelectionChanged, "sequence", [=] (const UIEvent& event)
	{
		animationDisplay->setSequence(event.getStringData());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "direction", [=] (const UIEvent& event)
	{
		animationDisplay->setDirection(event.getStringData());
	});
}

void AnimationEditor::loadAssetData()
{
	const auto animation = std::dynamic_pointer_cast<const Animation>(resource);
	const auto sprite = std::dynamic_pointer_cast<const SpriteResource>(resource);
	const auto texture = std::dynamic_pointer_cast<const Texture>(resource);

	if (animation) {
		animationDisplay->setAnimation(animation);
	} else if (sprite) {
		animationDisplay->setSprite(sprite);
	} else if (texture) {
		animationDisplay->setTexture(texture);
	}

	if (animation) {
		auto sequenceList = getWidgetAs<UIDropdown>("sequence");
		sequenceList->setOptions(animation->getSequenceNames());

		auto directionList = getWidgetAs<UIDropdown>("direction");
		directionList->setOptions(animation->getDirectionNames());
	} else {
		getWidget("animControls")->setActive(false);
	}
}

AnimationEditorDisplay::AnimationEditorDisplay(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	boundsSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceVSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceHSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	pivotSprite.setImage(resources, "ui/pivot.png").setColour(Colour4f(1, 0, 1));
}

void AnimationEditorDisplay::setZoom(float z)
{
	zoom = z;
	updateBounds();
}

void AnimationEditorDisplay::setAnimation(std::shared_ptr<const Animation> a)
{
	animation = std::move(a);
	animationPlayer.setAnimation(animation);
	bounds = Rect4f(animation->getBounds());
	updateBounds();
}

void AnimationEditorDisplay::setSprite(std::shared_ptr<const SpriteResource> sprite)
{
	origSprite.setImage(*sprite, resources.get<MaterialDefinition>("Halley/Sprite"));
	const auto origin = -origSprite.getAbsolutePivot() - Vector2f(origSprite.getOuterBorder().xy());
	const auto sz = origSprite.getUncroppedSize();
	bounds = Rect4f(origin, origin + sz);
}

void AnimationEditorDisplay::setTexture(std::shared_ptr<const Texture> texture)
{
	origSprite.setImage(texture, resources.get<MaterialDefinition>("Halley/Sprite"))
		.setTexRect(Rect4f(0, 0, 1, 1))
		.setColour(Colour4f(1, 1, 1, 1))
		.setSize(Vector2f(texture->getSize()));
	bounds = Rect4f(Vector2f(), origSprite.getSize());
}

void AnimationEditorDisplay::setSequence(const String& sequence)
{
	animationPlayer.setSequence(sequence);
}

void AnimationEditorDisplay::setDirection(const String& direction)
{
	animationPlayer.setDirection(direction);
}

const Rect4f& AnimationEditorDisplay::getBounds() const
{
	return bounds;
}

Vector2f AnimationEditorDisplay::getMousePos() const
{
	return mousePos;
}

void AnimationEditorDisplay::update(Time t, bool moved)
{
	updateBounds();

	if (animation) {
		animationPlayer.update(t);
		animationPlayer.updateSprite(origSprite);
	}

	const Vector2f pivotPos = imageToScreenSpace(-bounds.getTopLeft());;

	drawSprite = origSprite.clone().setPos(pivotPos).setScale(zoom).setNotSliced();
	pivotSprite.setPos(pivotPos);
	boundsSprite.setPos(getPosition()).scaleTo(bounds.getSize() * zoom);

	if (origSprite.isSliced()) {
		auto slices = Vector4f(origSprite.getSlices());
		nineSliceVSprite.setVisible(true).setPos(getPosition() + Vector2f(0, slices.y) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(0, slices.w + slices.y)) * zoom));
		nineSliceHSprite.setVisible(true).setPos(getPosition() + Vector2f(slices.x, 0) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(slices.x + slices.z, 0)) * zoom));
	} else {
		nineSliceVSprite.setVisible(false);
		nineSliceHSprite.setVisible(false);
	}
}

void AnimationEditorDisplay::draw(UIPainter& painter) const
{
	painter.draw(drawSprite);
	painter.draw(boundsSprite);
	painter.draw(pivotSprite);
	if (nineSliceHSprite.isVisible()) {
		painter.draw(nineSliceHSprite);
	}
	if (nineSliceVSprite.isVisible()) {
		painter.draw(nineSliceVSprite);
	}
}

void AnimationEditorDisplay::onMouseOver(Vector2f mousePos)
{
	this->mousePos = screenToImageSpace(mousePos);
}

void AnimationEditorDisplay::updateBounds()
{
	setMinSize(bounds.getSize() * zoom);
}

Vector2f AnimationEditorDisplay::imageToScreenSpace(Vector2f pos) const
{
	return getPosition() + zoom * pos;
}

Vector2f AnimationEditorDisplay::screenToImageSpace(Vector2f pos) const
{
	return (pos - getPosition()) / zoom;
}
