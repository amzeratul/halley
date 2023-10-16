#include "variant_gizmo.h"

using namespace Halley;

VariantGizmo::VariantGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
}

std::shared_ptr<UIWidget> VariantGizmo::makeUI()
{
	auto ui = factory.makeUI("halley/variants_gizmo");
	return ui;
}

bool VariantGizmo::onKeyPress(KeyboardKeyPress key)
{
	// TODO
	return false;
}
