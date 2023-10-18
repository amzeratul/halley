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
	ui = factory.makeUI("halley/variants_gizmo");

	variantsList = ui->getWidgetAs<UIList>("variants");

	populateVariants();
	populateVariantInfo();

	ui->setHandle(UIEventType::ButtonClicked, "add", [=] (const UIEvent& event)
	{
		addVariant();
	});

	ui->setHandle(UIEventType::ButtonClicked, "remove", [=] (const UIEvent& event)
	{
		removeVariant();
	});

	ui->setHandle(UIEventType::ListSelectionChanged, "variants", [=](const UIEvent& event)
	{
		populateVariantInfo();
	});

	ui->setHandle(UIEventType::ListItemsSwapped, "variants", [=](const UIEvent& event)
	{
		// TODO
	});

	return ui;
}

void VariantGizmo::populateVariants()
{
	auto& variantsNode = sceneEditorWindow.getGameData("variants");
	variantsNode.ensureType(ConfigNodeType::Sequence);
	auto& variants = variantsNode.asSequence();

	if (variants.empty()) {
		variants.push_back(SceneVariant("default").toConfigNode());
	}

	for (auto& variantNode: variants) {
		const auto variant = SceneVariant(variantNode);

		auto widget = factory.makeUI("halley/variant_entry");
		widget->getWidgetAs<UILabel>("name")->setText(LocalisedString::fromUserString(variant.id));
		variantsList->addItem(variant.id, widget);
	}
}

void VariantGizmo::populateVariantInfo()
{
	auto& variantsNode = sceneEditorWindow.getGameData("variants");
	variantsNode.ensureType(ConfigNodeType::Sequence);
	auto& variantNode = variantsNode.asSequence()[variantsList->getSelectedOption()];
	const auto variant = SceneVariant(variantNode);
	ui->getWidgetAs<UITextInput>("id")->setText(variant.id);
	ui->getWidgetAs<UITextInput>("conditions")->setText(variant.conditions.getExpression());
}

void VariantGizmo::addVariant()
{
	// TODO
}

void VariantGizmo::removeVariant()
{
	// TODO
}
