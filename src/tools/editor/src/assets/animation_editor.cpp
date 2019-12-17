#include "animation_editor.h"
using namespace Halley;

AnimationEditor::AnimationEditor(UIFactory& factory, Resources& resources, const String& animationId)
	: UIWidget("animationEditor", {}, UISizer())
{
	add(factory.makeUI("ui/halley/animation_editor"), 1);

	const auto anim = resources.get<Animation>(animationId);
	const auto bounds = Rect4f(anim->getBounds());

	auto animationDisplay = getWidgetAs<UIAnimation>("display");
	animationDisplay->getPlayer().setAnimation(anim);
	animationDisplay->setMinSize(bounds.getSize());
	animationDisplay->setOffset(-bounds.getTopLeft());
}
