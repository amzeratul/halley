#include "entity_editor.h"

#include "choose_asset_window.h"
#include "entity_editor_factories.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "scene_editor_window.h"
#include "src/ui/select_asset_widget.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
{
	makeUI();
	reloadEntity();
}

void EntityEditor::setEntityEditorFactory(std::shared_ptr<EntityEditorFactory> factory)
{
	entityEditorFactory = std::move(factory);
	if (entityEditorFactory) {
		entityEditorFactory->setEntityEditor(*this);
	}
}

void EntityEditor::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
}

void EntityEditor::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void EntityEditor::update(Time t, bool moved)
{
	if (ecsData && ecsData->getRevision() != ecsDataRevision) {
		ecsDataRevision = ecsData->getRevision();
		needToReloadUI = true;
	}
	
	if (needToReloadUI) {
		reloadEntity();
		needToReloadUI = false;
	}
}

void EntityEditor::setSceneEditorWindow(SceneEditorWindow& editor)
{
	sceneEditor = &editor;
	entityIcons = &editor.getEntityIcons();

	auto icons = entityIcons->getEntries();
	std::vector<UIDropdown::Entry> entries;
	entries.reserve(icons.size());
	for (const auto& icon: icons) {
		entries.emplace_back(icon.id, LocalisedString::fromUserString(icon.name), icon.icon);
	}
	entityIcon->setOptions(std::move(entries));
}

void EntityEditor::setECSData(ECSData& ecs)
{
	ecsData = &ecs;
	ecsDataRevision = ecsData->getRevision();
}

void EntityEditor::makeUI()
{
	add(factory.makeUI("ui/halley/entity_editor"), 1);
	fields = getWidget("fields");
	fields->setMinSize(Vector2f(300, 20));

	entityName = getWidgetAs<UITextInput>("entityName");
	prefabName = getWidgetAs<SelectAssetWidget>("prefabName");
	entityIcon = getWidgetAs<UIDropdown>("entityIcon");

	setHandle(UIEventType::ButtonClicked, "addComponentButton", [=](const UIEvent& event)
	{
		addComponent();
	});

	setHandle(UIEventType::TextSubmit, "entityName", [=] (const UIEvent& event)
	{
		setName(event.getStringData());
	});

	setHandle(UIEventType::TextChanged, "prefabName", [=] (const UIEvent& event)
	{
		setPrefabName(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "editPrefab", [=](const UIEvent& event)
	{
		editPrefab();
	});

	setHandle(UIEventType::DropboxSelectionChanged, "entityIcon", [=](const UIEvent& event)
	{
		setIcon(event.getStringData());
	});
}

bool EntityEditor::loadEntity(const String& id, EntityData& data, const Prefab* prefab, bool force, Resources& resources)
{
	Expects(ecsData);

	gameResources = &resources;
	prefabName->setGameResources(*gameResources);

	if (currentId == id && currentEntityData == &data && !force) {
		return false;
	}

	entityEditorFactory->setGameResources(resources);
	currentEntityData = &data;
	prefabData = prefab;
	currentId = id;
	isPrefab = !!prefabData;

	reloadEntity();

	// Only do this after reloadEntity, since factories will (annoyingly) modify entityData
	prevEntityData = EntityData(*currentEntityData);

	return true;
}

void EntityEditor::unloadEntity()
{
	currentEntityData = nullptr;
	prevEntityData = EntityData();
	prefabData = nullptr;
	currentId = "";
	isPrefab = false;
	reloadEntity();
}

void EntityEditor::reloadEntity()
{
	getWidget("entityHeader")->setActive(currentEntityData && !isPrefab);
	getWidget("prefabHeader")->setActive(currentEntityData && isPrefab);
	getWidget("addComponentButton")->setActive(currentEntityData);
	fields->clear();
	componentWidgets.clear();

	if (currentEntityData) {
		for (auto& c: getEntityData().getComponents()) {
			loadComponentData(c.first, c.second);
		}
		prevEntityData = EntityData(*currentEntityData);

		setCanSendEvents(false);
		if (isPrefab) {
			prefabName->setValue(getEntityData().getPrefab());
		} else {
			entityName->setText(getEntityData().getName());
			entityIcon->setSelectedOption(getEntityData().getIcon());
		}
		setCanSendEvents(true);
	}
}

void EntityEditor::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	sendEventDown(UIEvent(UIEventType::ReloadData, componentName + ":" + fieldName));
	onEntityUpdated();
}

void EntityEditor::loadComponentData(const String& componentType, ConfigNode& data)
{
	auto componentUI = factory.makeUI("ui/halley/entity_editor_component");
	componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));
	componentUI->setHandle(UIEventType::ButtonClicked, "deleteComponentButton", [=] (const UIEvent& event)
	{
		deleteComponent(componentType);
	});

	auto componentFields = componentUI->getWidget("componentFields");
	
	const auto iter = ecsData->getComponents().find(componentType);
	if (iter != ecsData->getComponents().end()) {
		const auto& componentData = iter->second;
		for (auto& member: componentData.members) {
			if (member.canEdit) {
				ComponentFieldParameters parameters(componentType, ComponentDataRetriever(data, member.name), member.defaultValue);
				auto field = entityEditorFactory->makeField(member.type.name, parameters, member.collapse ? ComponentEditorLabelCreation::Never : ComponentEditorLabelCreation::Always);
				if (field) {
					componentFields->add(field);
				}
			}
		}
	}

	setComponentColour(componentType, *componentUI);
	
	fields->add(componentUI);
	componentWidgets[componentType] = componentUI;
}

std::set<String> EntityEditor::getComponentsOnEntity() const
{
	// Components already on this entity
	std::set<String> existingComponents;
	auto& components = getEntityData().getComponents();
	for (auto& kv: components) {
		existingComponents.insert(kv.first);
	}
	return existingComponents;
}

std::set<String> EntityEditor::getComponentsOnPrefab() const
{
	std::set<String> prefabComponents;
	if (isPrefab) {
		const auto& comps = prefabData->getEntityData().getComponents();
		for (const auto& [k, v]: comps) {
			prefabComponents.insert(k);
		}
	}
	return prefabComponents;
}

void EntityEditor::addComponent()
{
	auto existingComponents = getComponentsOnEntity();
	auto prefabComponents = getComponentsOnPrefab();

	// Generate all available names
	std::vector<String> componentNames;
	for (const auto& c: ecsData->getComponents()) {
		if (existingComponents.find(c.first) == existingComponents.end()) {
			if (!isPrefab || prefabComponents.find(c.first) != prefabComponents.end()) {
				componentNames.push_back(c.first);
			}
		}
	}
	std::sort(componentNames.begin(), componentNames.end());

	getRoot()->addChild(std::make_shared<AddComponentWindow>(factory, componentNames, [=] (std::optional<String> result)
	{
		if (result) {
			addComponent(result.value());
		}
	}));
}

void EntityEditor::addComponent(const String& name)
{
	// Dependencies
	const auto iter = ecsData->getComponents().find(name);
	if (iter == ecsData->getComponents().end()) {
		throw Exception("Unknown component type: " + name, HalleyExceptions::Tools);
	}
	const auto& deps = iter->second.componentDependencies;

	// Get list
	auto& components = getEntityData().getComponents();

	// Insert dependencies, if needed
	auto existingComponents = getComponentsOnEntity();
	auto prefabComponents = getComponentsOnPrefab();
	for (const auto& dep: deps) {
		if (existingComponents.find(dep) == existingComponents.end() && prefabComponents.find(dep) == prefabComponents.end()) {
			addComponent(dep);
		}
	}
	
	// Insert
	components.emplace_back(name, ConfigNode::MapType());

	// Reload
	needToReloadUI = true;	
	onEntityUpdated();
}

void EntityEditor::deleteComponent(const String& name)
{
	auto& components = getEntityData().getComponents();
	
	for (size_t i = 0; i < components.size(); ++i) {
		if (components[i].first == name) {
			components.erase(components.begin() + i);

			needToReloadUI = true;
			sceneEditor->onComponentRemoved(name);
			onEntityUpdated();
			return;
		}
	}
}

void EntityEditor::setName(const String& name)
{
	if (!isPrefab && getName() != name) {
		getEntityData().setName(name);
		onEntityUpdated();
	}
}

void EntityEditor::setIcon(const String& icon)
{
	if (!isPrefab && getEntityData().getIcon() != icon) {
		getEntityData().setIcon(icon);
		onEntityUpdated();
	}
}

void EntityEditor::setDefaultName(const String& name, const String& prevName)
{
	const auto oldName = getName();
	if (oldName.isEmpty() || oldName == prevName) {
		entityName->setText(name);
		setName(name);
	}
}

bool EntityEditor::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::T, KeyMods::Ctrl)) {
		addComponent();
		return true;
	}

	return false;
}

String EntityEditor::getName() const
{
	return isPrefab ? "" : getEntityData().getName();
}

void EntityEditor::setPrefabName(const String& prefab)
{
	if (isPrefab) {
		const String oldPrefab = getEntityData().getPrefab();
		if (oldPrefab != prefab) {
			getEntityData().setPrefab(prefab);
			onEntityUpdated();
		}
	}
}

void EntityEditor::editPrefab()
{
	if (isPrefab) {
		const String prefab = getEntityData().getPrefab();
		sceneEditor->openEditPrefabWindow(prefab);
	}
}

void EntityEditor::onEntityUpdated()
{
	sceneEditor->onEntityModified(currentId, prevEntityData, *currentEntityData);
	prevEntityData = EntityData(*currentEntityData);
}

void EntityEditor::setTool(const String& tool, const String& componentName, const String& fieldName)
{
	sceneEditor->setTool(tool, componentName, fieldName);
}

EntityData& EntityEditor::getEntityData()
{
	return *currentEntityData;
}

const EntityData& EntityEditor::getEntityData() const
{
	return *currentEntityData;
}

void EntityEditor::setHighlightedComponents(std::vector<String> componentNames)
{
	if (componentNames != highlightedComponents) {
		highlightedComponents = std::move(componentNames);

		for (auto& [compName, widget]: componentWidgets) {
			setComponentColour(compName, *widget);
		}
	}
}

void EntityEditor::setComponentColour(const String& name, UIWidget& component)
{
	const bool highlighted = std_ex::contains(highlightedComponents, name);
	
	const auto colour = factory.getColourScheme()->getColour(highlighted ? "ui_listSelected" : "ui_staticBox");
	auto capsule = component.getWidgetAs<UIImage>("capsule");
	capsule->getSprite().setColour(colour);
}

EntityEditorFactory::EntityEditorFactory(UIFactory& factory)
	: factory(factory)
{
	resetFieldFactories();
	makeContext();
}

std::shared_ptr<IUIElement> EntityEditorFactory::makeLabel(const String& text) const
{
	auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromUserString(text));
	label->setMaxWidth(100);
	label->setMarquee(true);
	auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
	labelBox->add(label);
	return labelBox;
}

std::shared_ptr<IUIElement> EntityEditorFactory::makeField(const String& rawFieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const
{
	auto [fieldType, typeParams] = parseType(rawFieldType);
	parameters.typeParameters = std::move(typeParams);
		
	const auto iter = fieldFactories.find(fieldType);
	auto* compFieldFactory = iter != fieldFactories.end() ? iter->second.get() : nullptr;

	if (createLabel == ComponentEditorLabelCreation::Always && compFieldFactory && compFieldFactory->canCreateLabel()) {
		return compFieldFactory->createLabelAndField(*context, parameters);
	} else if (createLabel != ComponentEditorLabelCreation::Never && compFieldFactory && compFieldFactory->isNested()) {
		auto field = factory.makeUI("ui/halley/entity_editor_compound_field");
		field->getWidgetAs<UILabel>("fieldName")->setText(LocalisedString::fromUserString(parameters.data.getName()));
		field->getWidget("fields")->add(compFieldFactory->createField(*context, parameters));
		return field;
	} else {
		auto container = std::make_shared<UISizer>();
		if (createLabel == ComponentEditorLabelCreation::Always) {
			container->add(makeLabel(parameters.data.getName()), 0, {}, UISizerAlignFlags::CentreVertical);
		}

		if (compFieldFactory) {
			container->add(compFieldFactory->createField(*context, parameters), 1, Vector4f(), UISizerAlignFlags::Top | UISizerFillFlags::FillHorizontal);
		} else {
			container->add(std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromHardcodedString("N/A")));
		}

		return container;
	}

	return {};
}

ConfigNode EntityEditorFactory::getDefaultNode(const String& fieldType) const
{
	const auto iter = fieldFactories.find(fieldType);
	if (iter == fieldFactories.end()) {
		return ConfigNode();
	} else {
		return iter->second->getDefaultNode();
	}
}

void EntityEditorFactory::addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}

void EntityEditorFactory::resetFieldFactories()
{
	fieldFactories.clear();
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
}

void EntityEditorFactory::setEntityEditor(IEntityEditor& editor)
{
	entityEditor = &editor;
	makeContext();
}

void EntityEditorFactory::setGameResources(Resources& resources)
{
	gameResources = &resources;
	makeContext();
}

std::pair<String, std::vector<String>> EntityEditorFactory::parseType(const String& type) const
{
	// This will split the C++ type for templates, e.g.:
	// std::optional<Halley::String> -> "std::optional<>", {"Halley::String"}
	// std::map<int, Halley::Colour4f> -> "std::map<>", {"int", "Halley::Colour4f"}
	// Multiple levels of nesting are not supported atm

	const auto openPos = type.find('<');
	const auto closePos = type.find_last_of('>');
	if (openPos != String::npos && closePos != String::npos) {
		auto base = type.left(openPos) + "<>";
		auto remain = type.substr(openPos + 1, closePos - openPos - 1).split(',');
		return {base, remain};
	}
	return {type, {}};
}

void EntityEditorFactory::makeContext()
{
	context = std::make_unique<ComponentEditorContext>(*this, entityEditor, factory, *gameResources);
}
