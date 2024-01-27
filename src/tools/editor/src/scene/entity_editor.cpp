#include "entity_editor.h"

#include <components/script_target_component.h>

#include "choose_window.h"
#include "entity_editor_factories.h"
#include "entity_validator_ui.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "scene_editor_window.h"
#include "halley/entity/components/transform_2d_component.h"
#include "src/assets/graph/script_graph_editor.h"
#include "src/ui/project_window.h"
#include "src/ui/select_asset_widget.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
{
	makeUI();
	reloadEntity();
}

void EntityEditor::setEntityEditorFactory(EntityEditorFactoryRoot* rootFactory)
{
	if (rootFactory) {
		entityEditorFactory = std::make_shared<EntityEditorFactory>(*rootFactory, static_cast<IEntityEditorCallbacks*>(this));
	} else {
		entityEditorFactory.reset();
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

void EntityEditor::setSceneEditorWindow(SceneEditorWindow& editor, const HalleyAPI& api)
{
	sceneEditor = &editor;
	entityIcons = &editor.getEntityIcons();
	this->api = &api;

	entityValidatorUI->setValidator(&editor.getEntityValidator());
	entityValidatorUI->setSceneEditorWindow(&editor);

	auto icons = entityIcons->getEntries();
	Vector<UIDropdown::Entry> entries;
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
	add(factory.makeUI("halley/entity_editor"), 1);
	fields = getWidget("fields");
	fields->setMinSize(Vector2f(300, 20));

	entityName = getWidgetAs<UITextInput>("entityName");
	prefabName = getWidgetAs<SelectAssetWidget>("prefabName");
	entityIcon = getWidgetAs<UIDropdown>("entityIcon");
	variant = getWidgetAs<UIDropdown>("variant");

	entityValidatorUI = getWidgetAs<EntityValidatorUI>("entityValidatorUI");

	setHandle(UIEventType::ButtonClicked, "addComponentButton", [=](const UIEvent& event)
	{
		addComponent();
	});

	setHandle(UIEventType::ButtonClicked, "copyComponentsButton", [=](const UIEvent& event)
	{
		copyAllComponentsToClipboard();
	});

	setHandle(UIEventType::ButtonClicked, "pasteComponentsButton", [=](const UIEvent& event)
	{
		pasteComponentsFromClipboard();
	});

	setHandle(UIEventType::CheckboxUpdated, "selectable", [=] (const UIEvent& event)
	{
		setSelectable(event.getBoolData());
	});

	setHandle(UIEventType::CheckboxUpdated, "serializable", [=] (const UIEvent& event)
	{
		setSerializable(event.getBoolData());
	});

	setHandle(UIEventType::CheckboxUpdated, "enabled", [=] (const UIEvent& event)
	{
		setEntityEnabled(event.getBoolData());
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

	setHandle(UIEventType::DropdownSelectionChanged, "entityIcon", [=](const UIEvent& event)
	{
		setIcon(event.getStringData());
	});

	setHandle(UIEventType::DropdownSelectionChanged, "variant", [=](const UIEvent& event)
	{
		setVariant(event.getStringData());
	});
}

bool EntityEditor::loadEntity(const String& id, EntityData& data, const Prefab* prefab, bool force, Resources& resources)
{
	Expects(ecsData);

	gameResources = &resources;

	if (currentId == id && currentEntityData == &data && !force) {
		return false;
	}

	currentEntityData = &data;
	prefabData = prefab;
	currentId = id;
	isPrefab = !!prefabData;

	reloadEntity();

	// Only do this after reloadEntity, since factories will (annoyingly) modify entityData
	prevEntityData = EntityData(*currentEntityData);

	return true;
}

void EntityEditor::unloadEntity(bool becauseHasMultiple)
{
	currentEntityData = nullptr;
	prevEntityData = EntityData();
	prefabData = nullptr;
	currentId = "";
	isPrefab = false;
	unloadedBecauseHasMultiple = becauseHasMultiple;
	reloadEntity();
}

void EntityEditor::reloadEntity()
{
	loadVariants();

	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromHardcodedString(isPrefab ? "Prefab" : "Entity"));
	getWidget("scrollBarPane")->setActive(currentEntityData);
	getWidget("entityFields")->setActive(currentEntityData && !isPrefab);
	getWidget("prefabFields")->setActive(currentEntityData && isPrefab);
	getWidget("componentButtons")->setActive(currentEntityData);

	auto msg = getWidgetAs<UILabel>("message");
	msg->setActive(!currentEntityData);
	msg->setText(LocalisedString::fromHardcodedString(unloadedBecauseHasMultiple ? "Multiple Entities Selected" : "No Entities Selected"));

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
			variant->setSelectedOption(getEntityData().getVariant().isEmpty() ? "default" : getEntityData().getVariant());
		}
		getWidgetAs<UICheckbox>("selectable")->setChecked(!getEntityData().getFlag(EntityData::Flag::NotSelectable));
		getWidgetAs<UICheckbox>("serializable")->setChecked(!getEntityData().getFlag(EntityData::Flag::NotSerializable));
		getWidgetAs<UICheckbox>("enabled")->setChecked(!getEntityData().getFlag(EntityData::Flag::Disabled));
		setCanSendEvents(true);

		entityValidatorUI->setEntity(*currentEntityData, *this, *gameResources);
	} else {
		entityValidatorUI->unloadEntity();
	}
}

void EntityEditor::unloadIcons()
{
	if (entityIcon) {
		entityIcon->setOptions(Vector<UIDropdown::Entry>{});
	}
}

void EntityEditor::unloadValidator()
{
	entityValidatorUI->setValidator(nullptr);
}

void EntityEditor::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	if (currentEntityData) {
		sendEventDown(UIEvent(UIEventType::ReloadData, componentName + ":" + fieldName));
		refreshEntityData();
	}
}

void EntityEditor::onFieldChangedProcedurally(const String& componentName, const String& fieldName)
{
	if (currentEntityData) {
		sendEventDown(UIEvent(UIEventType::ReloadData, componentName + ":" + fieldName));
		onEntityUpdated();
		sceneEditor->refreshGizmos();
	}
}

void EntityEditor::loadComponentData(const String& componentType, ConfigNode& data)
{
	auto componentUI = factory.makeUI("halley/entity_editor_component");
	componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));
	componentUI->setHandle(UIEventType::ButtonClicked, "deleteComponentButton", [=] (const UIEvent& event)
	{
		deleteComponent(componentType);
	});
	componentUI->setHandle(UIEventType::ButtonClicked, "copyComponentButton", [=] (const UIEvent& event)
	{
		const bool ctrlHeld = (static_cast<int>(event.getKeyMods()) & static_cast<int>(KeyMods::Ctrl)) != 0;
		copyComponentToClipboard(componentType, ctrlHeld);
	});

	auto componentFields = componentUI->getWidget("componentFields");
	
	const auto iter = ecsData->getComponents().find(componentType);
	if (iter != ecsData->getComponents().end()) {
		const auto& componentData = iter->second;
		for (auto& member: componentData.members) {
			if (std_ex::contains(member.serializationTypes, EntitySerialization::Type::Prefab) && !member.hideInEditor) {
				auto type = member.type.name;
				ConfigNode options;
				if (type == "float" && member.range) {
					type = "Halley::Range<" + type + ">";
					options = ConfigNode::MapType();
					options["start"] = member.range->start;
					options["end"] = member.range->end;
				}
				
				ComponentFieldParameters parameters(componentType, ComponentDataRetriever(data, member.name, member.displayName.isEmpty() ? member.name : member.displayName), member.defaultValue, {}, std::move(options));
				auto field = entityEditorFactory->makeField(type, parameters, member.collapse ? ComponentEditorLabelCreation::Never : ComponentEditorLabelCreation::Always);
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

void EntityEditor::loadVariants()
{
	if (!variant) {
		return;
	}

	Vector<String> options;

	if (sceneEditor && !sceneEditor->getCurrentAssetId().isEmpty()) {
		auto variants = sceneEditor->getGameData("variants").asVector<SceneVariant>({});
		for (const auto& variant: variants) {
			options.push_back(variant.id);
		}
	}

	if (options.empty()) {
		options.push_back("default");
	}
	variant->setOptions(options);
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
	Vector<String> componentNames;
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
			addComponent(result.value(), ConfigNode::MapType());
		}
	}));
}

void EntityEditor::addComponent(const String& name, ConfigNode data)
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
			addComponent(dep, ConfigNode::MapType());
		}
	}
	
	// Insert
	components.emplace_back(name, std::move(data));

	// Reload
	needToReloadUI = true;	
	onEntityUpdated();
	sceneEditor->validateAllEntities();
}

void EntityEditor::deleteComponent(const String& name)
{
	auto& components = getEntityData().getComponents();
	
	for (size_t i = 0; i < components.size(); ++i) {
		if (components[i].first == name) {
			components.erase(components.begin() + i);

			needToReloadUI = true;
			sceneEditor->onComponentRemoved(name);
			sceneEditor->validateAllEntities();
			onEntityUpdated();
			return;
		}
	}
}

void EntityEditor::copyAllComponentsToClipboard()
{
	auto& componentDatas = getEntityData().getComponents();
	ConfigNode components = ConfigNode::SequenceType();

	for (size_t i = 0; i < componentDatas.size(); ++i) {
		components.asSequence().push_back(serializeComponent(componentDatas[i].first, componentDatas[i].second));
	}

	copyComponentsToClipboard(std::move(components));
}

void EntityEditor::copyComponentToClipboard(const String& name, bool append)
{
	auto& componentDatas = getEntityData().getComponents();
	ConfigNode components = ConfigNode::SequenceType();

	if (append) {
		auto existingComponents = getComponentsFromClipboard();
		if (existingComponents.getType() == ConfigNodeType::Sequence) {
			components = std::move(existingComponents);
		}
	}

	for (size_t i = 0; i < componentDatas.size(); ++i) {
		if (componentDatas[i].first == name) {
			auto data = serializeComponent(name, componentDatas[i].second);
			bool found = false;
			for (auto& existingComp: components.asSequence()) {
				for (auto& entry: existingComp.asMap()) {
					if (entry.first == name) {
						entry.second = std::move(data);
						found = true;
						break;
					}
				}
			}

			if (!found) {
				components.asSequence().push_back(std::move(data));
			}
			break;
		}
	}

	copyComponentsToClipboard(std::move(components));
}

void EntityEditor::copyComponentsToClipboard(ConfigNode components)
{
	auto clipboard = api->system->getClipboard();
	if (!clipboard) {
		return;
	}

	ConfigNode result = ConfigNode::MapType();
	result["components"] = std::move(components);
	YAMLConvert::EmitOptions options;
	clipboard->setData(YAMLConvert::generateYAML(result, options));
}

ConfigNode EntityEditor::serializeComponent(const String& name, const ConfigNode& data)
{
	ConfigNode component = ConfigNode::MapType();
	component[name] = ConfigNode(data);

	return component;
}

ConfigNode EntityEditor::getComponentsFromClipboard()
{
	auto clipboard = api->system->getClipboard();
	if (!clipboard) {
		return {};
	}

	auto data = clipboard->getStringData();
	if (!data) {
		return {};
	}

	ConfigFile file;
	try {
		YAMLConvert::parseConfig(file, gsl::as_bytes(gsl::span<const char>(data->c_str(), data->length())));
		if (isValidComponents(file.getRoot())) {
			return std::move(file.getRoot()["components"]);
		}
	} catch (...) {}

	return {};
}

void EntityEditor::pasteComponentsFromClipboard()
{
	const auto components = getComponentsFromClipboard();
	if (components.getType() == ConfigNodeType::Sequence) {
		pasteComponents(components);
	}
}

bool EntityEditor::isValidComponents(const ConfigNode& data)
{
	if (data.hasKey("components")) {
		auto& components = data["components"];
		if (components.getType() == ConfigNodeType::Sequence) {
			return true;
		}
	}
	return false;
}

void EntityEditor::pasteComponents(const ConfigNode& data)
{
	for (const auto& comp: data.asSequence()) {
		if (comp.getType() == ConfigNodeType::Map) {
			for (const auto& [k, v]: comp.asMap()) {
				pasteComponent(k, ConfigNode(v));
			}
		}
	}
}

void EntityEditor::pasteComponent(const String& name, ConfigNode data)
{
	auto& entityData = getEntityData();
	auto& components = entityData.getComponents();
	
	for (size_t i = 0; i < components.size(); ++i) {
		if (components[i].first == name) {
			components[i].second = std::move(data);

			needToReloadUI = true;
			onEntityUpdated();
			return;
		}
	}

	// Not found, add it instead
	if (entityData.getPrefab().isEmpty()) {
		addComponent(name, std::move(data));
	}
}

void EntityEditor::setName(const String& name, bool markModified)
{
	if (!isPrefab && getName() != name) {
		getEntityData().setName(name);
		if (markModified) {
			onEntityUpdated();
		}
	}
}

void EntityEditor::setIcon(const String& icon)
{
	if (!isPrefab && getEntityData().getIcon() != icon) {
		getEntityData().setIcon(icon);
		onEntityUpdated();
	}
}

void EntityEditor::setVariant(const String& variant)
{
	auto value = variant == "default" ? "" : variant;
	if (getEntityData().getVariant() != value) {
		getEntityData().setVariant(value);
		onEntityUpdated();
	}
}

void EntityEditor::setDefaultName(const String& name, const String& prevName)
{
	Expects(currentEntityData);

	const auto oldName = getName();
	if (oldName.isEmpty() || oldName == prevName) {
		entityName->setText(name);
		setName(name, false);
	}
}

void EntityEditor::focusRenameEntity()
{
	if (entityName->isActiveInHierarchy()) {
		getRoot()->setFocus(entityName);
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
	return isPrefab || !currentEntityData ? "" : currentEntityData->getName();
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

void EntityEditor::setSelectable(bool selectable)
{
	if (getEntityData().getFlag(EntityData::Flag::NotSelectable) == selectable) {
		getEntityData().setFlag(EntityData::Flag::NotSelectable, !selectable);
		onEntityUpdated();
	}
}

void EntityEditor::setSerializable(bool serializable)
{
	if (getEntityData().getFlag(EntityData::Flag::NotSerializable) == serializable) {
		getEntityData().setFlag(EntityData::Flag::NotSerializable, !serializable);
		onEntityUpdated();
	}
}

void EntityEditor::setEntityEnabled(bool enabled)
{
	if (getEntityData().getFlag(EntityData::Flag::Disabled) == enabled) {
		getEntityData().setFlag(EntityData::Flag::Disabled, !enabled);
		onEntityUpdated();
	}
}

void EntityEditor::editPrefab()
{
	if (isPrefab) {
		const String prefab = getEntityData().getPrefab();
		sceneEditor->openEditPrefabWindow(prefab);
	}
}

void EntityEditor::refreshEntityData()
{
	if (currentEntityData) {
		prevEntityData = EntityData(*currentEntityData);
		entityValidatorUI->refresh();
	}
}

void EntityEditor::onEntityUpdated(bool temporary)
{
	if (currentEntityData) {
		sceneEditor->onEntityModified(currentId, prevEntityData, *currentEntityData);
		refreshEntityData();
	}
}

void EntityEditor::setTool(const String& tool, const String& componentName, const String& fieldName)
{
	if (tool == "!scripting") {
		auto graph = std::make_shared<ScriptGraph>(getComponentData(componentName, fieldName));
		graph->setAssetId(String("embed:") + currentEntityData->getInstanceUUID());

		Vector<String> entityTargets;
		for (const auto& e: sceneEditor->getWorld().getEntities()) {
			if (const auto* target = e.tryGetComponent<ScriptTargetComponent>()) {
				entityTargets.emplace_back(target->id);
			}
		}

		auto scene = std::dynamic_pointer_cast<const Scene>(sceneEditor->getCurPrefab());

		auto scriptEditor = std::make_shared<ScriptGraphEditor>(factory, *gameResources, sceneEditor->getProjectWindow(), graph, nullptr, scene,
			[=, componentName=componentName, fieldName=fieldName] (bool accept, std::shared_ptr<ScriptGraph> graph)
		{
			if (accept) {
				getComponentData(componentName, fieldName) = graph->toConfigNode();
				onEntityUpdated();
			}
		}, std::move(entityTargets));
		sceneEditor->drillDownEditor(std::move(scriptEditor));
	} else if (tool == "!timeline") {
		auto timeline = std::make_shared<Timeline>(getComponentData(componentName, fieldName));
		sceneEditor->editTimeline(currentId, std::move(timeline));
	} else {
		sceneEditor->setTool(tool, componentName, fieldName);
	}
}

EntityData& EntityEditor::getEntityData()
{
	Expects(currentEntityData);
	return *currentEntityData;
}

const EntityData& EntityEditor::getEntityData() const
{
	return *currentEntityData;
}

ConfigNode& EntityEditor::getComponentData(const String& componentName, const String& fieldName)
{
	for (auto& comp : currentEntityData->getComponents()) {
		if (comp.first == componentName) {
			return comp.second[fieldName];
		}
	}
	static ConfigNode dummy;
	return dummy;
}

void EntityEditor::setHighlightedComponents(Vector<String> componentNames)
{
	if (componentNames != highlightedComponents) {
		highlightedComponents = std::move(componentNames);

		for (auto& [compName, widget]: componentWidgets) {
			setComponentColour(compName, *widget);
		}
	}
}

IProjectWindow& EntityEditor::getProjectWindow() const
{
	return sceneEditor->getProjectWindow();
}

namespace {
	void collectEntities(EntityList& entityList, const EntityTree& tree, const String& currentEntityId, bool isChildOfCurrentlySelected, String& longName, Vector<IEntityEditorCallbacks::EntityInfo>& dst)
	{
		if (!tree.entityId.isEmpty()) {
			isChildOfCurrentlySelected |= tree.entityId == currentEntityId;
			auto info = entityList.getEntityInfo(*tree.data);
			String name = isChildOfCurrentlySelected ? "(X) " : "";
			name = name + longName + info.name;
			longName = longName + info.name + "/";
			dst.push_back(IEntityEditorCallbacks::EntityInfo{ std::move(name), std::move(info.icon), UUID(tree.entityId) });
		}
		for (auto& c: tree.children) {
			collectEntities(entityList, c, currentEntityId, isChildOfCurrentlySelected, longName, dst);
		}
		if (!tree.entityId.isEmpty()) {
			longName = longName.substr(0, longName.substr(0, longName.length() - 1).find_last_of('/') + 1);
		}
	}
}

Vector<IEntityEditorCallbacks::EntityInfo> EntityEditor::getEntities() const
{
	Vector<EntityInfo> result;
	String longName;
	
	collectEntities(*sceneEditor->getEntityList(), sceneEditor->getSceneData()->getEntityTree(), currentId, false, longName, result);
	return result;
}

IEntityEditorCallbacks::EntityInfo EntityEditor::getEntityInfo(const UUID& uuid) const
{
	const auto data = sceneEditor->getSceneData()->tryGetEntityNodeData(uuid.toString());
	if (!data) {
		return {};
	}
	const auto info = sceneEditor->getEntityList()->getEntityInfo(data->getData());
	return { info.name, info.icon, uuid };
}

void EntityEditor::goToEntity(const UUID& uuid)
{
	sceneEditor->selectEntity(uuid.toString());
}

void EntityEditor::setComponentColour(const String& name, UIWidget& component)
{
	const bool highlighted = std_ex::contains(highlightedComponents, name);
	
	const auto colour = factory.getColourScheme()->getColour(highlighted ? "ui_listSelected" : "ui_staticBox");
	auto capsule = component.getWidgetAs<UIImage>("capsule");
	capsule->getSprite().setColour(colour);
}



EntityEditorFactoryRoot::EntityEditorFactoryRoot(ProjectWindow& projectWindow, UIFactory& factory)
	: projectWindow(projectWindow)
	, factory(factory)
{
}

void EntityEditorFactoryRoot::addFieldFactories(Vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}

void EntityEditorFactoryRoot::addStandardFieldFactories()
{
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
	addFieldFactories(EntityEditorFactories::getECSFactories(projectWindow.getProject().getECSData()));
}

void EntityEditorFactoryRoot::clear()
{
	fieldFactories.clear();
}

bool EntityEditorFactoryRoot::isEmpty() const
{
	return fieldFactories.empty();
}

void EntityEditorFactoryRoot::setGameResources(Resources& resources, const HalleyAPI& api)
{
	gameResources = &resources;
	this->api = &api;
}



EntityEditorFactory::EntityEditorFactory(EntityEditorFactoryRoot& root, IEntityEditorCallbacks* callbacks)
	: root(root)
	, context(makeContext(callbacks))
{
}

std::shared_ptr<IUIElement> EntityEditorFactory::makeLabel(const String& text) const
{
	auto label = std::make_shared<UILabel>("", root.factory.getStyle("labelLight"), LocalisedString::fromUserString(text));
	label->setMaxWidth(100);
	label->setMarquee(true);
	auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
	labelBox->add(label, 0, {}, UISizerAlignFlags::Centre);
	return labelBox;
}

std::shared_ptr<UIWidget> EntityEditorFactory::makeNestedField(const String& text) const
{
	auto field = root.factory.makeUI("halley/entity_editor_compound_field");
	field->getWidgetAs<UILabel>("fieldName")->setText(LocalisedString::fromUserString(text));
	return field;
}

std::shared_ptr<IUIElement> EntityEditorFactory::makeField(const String& rawFieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const
{
	auto [fieldType, typeParams] = parseType(rawFieldType);
	parameters.typeParameters = std::move(typeParams);

	IComponentEditorFieldFactory* compFieldFactory = nullptr;
	if (const auto additionalIter = additionalFieldFactories.find(fieldType); additionalIter != additionalFieldFactories.end()) {
		compFieldFactory = additionalIter->second.get();
	} else if (const auto iter = root.fieldFactories.find(fieldType); iter != root.fieldFactories.end()) {
		compFieldFactory = iter->second.get();
	}

	if (createLabel == ComponentEditorLabelCreation::Always && compFieldFactory && compFieldFactory->canCreateLabel()) {
		return compFieldFactory->createLabelAndField(*context, parameters);
	} else if (createLabel != ComponentEditorLabelCreation::Never && compFieldFactory && compFieldFactory->isNested()) {
		auto field = makeNestedField(parameters.data.getLabelName());
		field->getWidget("fields")->add(compFieldFactory->createField(*context, parameters));
		return field;
	} else {
		auto container = std::make_shared<UISizer>();
		if (createLabel == ComponentEditorLabelCreation::Always) {
			container->add(makeLabel(parameters.data.getLabelName()), 0, {}, UISizerAlignFlags::CentreVertical);
		}

		if (compFieldFactory) {
			container->add(compFieldFactory->createField(*context, parameters), 1, Vector4f(), UISizerAlignFlags::Top | UISizerFillFlags::FillHorizontal);
		} else {
			container->add(std::make_shared<UILabel>("", root.factory.getStyle("labelLight"), LocalisedString::fromHardcodedString("N/A")));
		}

		return container;
	}
}

ConfigNode EntityEditorFactory::getDefaultNode(const String& fieldType) const
{
	const auto iter = root.fieldFactories.find(fieldType);
	if (iter == root.fieldFactories.end()) {
		return ConfigNode();
	} else {
		return iter->second->getDefaultNode();
	}
}

void EntityEditorFactory::addFieldFactory(std::unique_ptr<IComponentEditorFieldFactory> factory)
{
	additionalFieldFactories[factory->getFieldType()] = std::move(factory);
}

std::pair<String, Vector<String>> EntityEditorFactory::parseType(const String& type) const
{
	// This will split the C++ type for templates, e.g.:
	// std::optional<Halley::String> -> "std::optional<>", {"Halley::String"}
	// std::map<int, Halley::Colour4f> -> "std::map<>", {"int", "Halley::Colour4f"}
	// Multiple levels of nesting are not supported atm

	const auto openPos = type.find('<');
	const auto closePos = type.find_last_of('>');
	if (openPos != String::npos && closePos != String::npos) {		
		auto base = type.left(openPos) + "<>";

		const auto remainSubstr = type.substr(openPos + 1, closePos - openPos - 1);
		if (remainSubstr.contains('<')) {
			return { base, {remainSubstr} };
		}
		
		auto remain = remainSubstr.split(',');
		return {base, remain};
	}
	return {type, {}};
}

std::unique_ptr<ComponentEditorContext> EntityEditorFactory::makeContext(IEntityEditorCallbacks* callbacks)
{
	return std::make_unique<ComponentEditorContext>(root.projectWindow, *this, callbacks, root.factory, root.gameResources, root.api);
}
