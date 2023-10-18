#include "variant_gizmo.h"

#include "src/assets/new_asset_window.h"

using namespace Halley;

VariantGizmo::VariantGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
	loadVariants();
}

std::shared_ptr<UIWidget> VariantGizmo::makeUI()
{
	ui = factory.makeUI("halley/variants_gizmo");

	variantsList = ui->getWidgetAs<UIList>("variants");

	populateVariants(0);
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
		const int a = event.getIntData();
		const int b = event.getIntData2();
		std::swap(variants[a], variants[b]);
		populateVariantInfo();
		saveVariants();
	});

	return ui;
}

void VariantGizmo::loadVariants()
{
	variants = sceneEditorWindow.getGameData("variants").asVector<SceneVariant>({});
	if (variants.empty()) {
		variants.push_back(SceneVariant("default"));
	}
}

void VariantGizmo::saveVariants()
{
	sceneEditorWindow.getGameData("variants") = variants;
	sceneEditorWindow.markModified();
}

void VariantGizmo::populateVariants(int startIdx)
{
	variantsList->clear();
	for (auto& variant: variants) {
		auto widget = factory.makeUI("halley/variant_entry");

		const auto selImage = widget->getWidgetAs<UIImage>("background");
		const auto normalCol = selImage->getSprite().getColour();
		const auto selCol = factory.getColourScheme()->getColour("ui_listSelected");
		const auto hoverCol = factory.getColourScheme()->getColour("ui_listHover");
		selImage->setHoverableSelectable(normalCol, hoverCol, selCol);

		widget->getWidgetAs<UILabel>("name")->setText(LocalisedString::fromUserString(variant.id));
		variantsList->addItem(variant.id, widget);
	}
	variantsList->setSelectedOption(startIdx);
}

void VariantGizmo::populateVariantInfo()
{
	auto& variant = variants[variantsList->getSelectedOption()];

	ui->bindData("id", variant.id, [this, &variant](String value)
	{
		variant.id = value;
		saveVariants();
		variantsList->setItemText(variantsList->getSelectedOption(), value);
	});
	ui->bindData("conditions", variant.conditions.getExpression(), [this, &variant](String value)
	{
		variant.conditions = value;
		saveVariants();
	});

	ui->getWidget("remove")->setEnabled(variant.id != "default");
	ui->getWidget("id")->setEnabled(variant.id != "default");
	ui->getWidget("conditions")->setEnabled(variant.id != "default");
}

void VariantGizmo::addVariant()
{
	ui->getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("New Variant"), "", "", [=](std::optional<String> newName)
	{
		if (newName) {
			variants.push_back(SceneVariant(*newName));
			populateVariants(static_cast<int>(variants.size()) - 1);
			saveVariants();
		}
	}));
}

void VariantGizmo::removeVariant()
{
	const int idx = variantsList->getSelectedOption();
	if (idx >= 0 && idx < static_cast<int>(variants.size())) {
		if (variants[idx].id != "default") {
			variants.erase(variants.begin() + idx);
			populateVariants(idx);
		}
	}
	saveVariants();
}
