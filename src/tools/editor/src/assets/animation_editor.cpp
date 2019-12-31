#include "animation_editor.h"
#include "halley/tools/project/project.h"
#include "halley/ui/widgets/ui_animation.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "src/ui/scroll_background.h"

using namespace Halley;

AnimationEditor::AnimationEditor(UIFactory& factory, Resources& resources, Project& project, const String& animationId)
	: UIWidget("animationEditor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	animation = resources.get<Animation>(animationId);
	setupWindow();
}

void AnimationEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/animation_editor"), 1);

	auto animationDisplay = getWidgetAs<AnimationEditorDisplay>("display");
	animationDisplay->setAnimation(animation);
	getWidgetAs<ScrollBackground>("scrollBackground")->setZoomListener([=] (float zoom)
	{
		animationDisplay->setZoom(zoom);
	});

	auto sequenceList = getWidgetAs<UIDropdown>("sequence");
	sequenceList->setOptions(animation->getSequenceNames());

	auto directionList = getWidgetAs<UIDropdown>("direction");
	directionList->setOptions(animation->getDirectionNames());

	setHandle(UIEventType::DropboxSelectionChanged, "sequence", [=] (const UIEvent& event)
	{
		animationDisplay->setSequence(event.getData());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "direction", [=] (const UIEvent& event)
	{
		animationDisplay->setDirection(event.getData());
	});
}

AnimationEditorDisplay::AnimationEditorDisplay(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	boundsSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	pivotSprite.setImage(resources, "ui/pivot.png").setColour(Colour4f(1, 0, 1));
}

void AnimationEditorDisplay::setZoom(float z)
{
	zoom = z;
	updateAnimation();
}

void AnimationEditorDisplay::setAnimation(std::shared_ptr<const Animation> a)
{
	animation = std::move(a);
	animationPlayer.setAnimation(animation);
	updateAnimation();
}

void AnimationEditorDisplay::setSequence(const String& sequence)
{
	animationPlayer.setSequence(sequence);
}

void AnimationEditorDisplay::setDirection(const String& direction)
{
	animationPlayer.setDirection(direction);
}

void AnimationEditorDisplay::update(Time t, bool moved)
{
	updateAnimation();

	animationPlayer.update(t);
	animationPlayer.updateSprite(sprite);

	const Vector2f pivotPos = getPosition() - bounds.getTopLeft() * zoom;

	sprite.setPos(pivotPos).setScale(zoom);
	pivotSprite.setPos(pivotPos);
	boundsSprite.setPos(getPosition()).scaleTo(bounds.getSize() * zoom);
}

void AnimationEditorDisplay::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(boundsSprite);
	painter.draw(pivotSprite);
}

void AnimationEditorDisplay::updateAnimation()
{
	bounds = Rect4f(animation->getBounds());
	setMinSize(bounds.getSize() * zoom);
}
