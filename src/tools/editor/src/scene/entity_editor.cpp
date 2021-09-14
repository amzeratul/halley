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

void EntityEditor::setSceneEditorWindow(SceneEditorWindow& editor, const HalleyAPI& api)
{
	sceneEditor = &editor;
	entityIcons = &editor.getEntityIcons();
	this->api = &api;
	prefabName->setSceneEditorWindow(editor);

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
	if (sceneEditor) {
		prefabName->setSceneEditorWindow(*sceneEditor);
	}
	entityIcon = getWidgetAs<UIDropdown>("entityIcon");

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

void EntityEditor::unloadIcons()
{
	entityIcon->clear();
	entityIcon.reset();
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
			if (member.canEdit) {
				auto type = member.type.name;
				if (type == "float" && member.range) {
					type = "Halley::Range<" + type + "," + toString(member.range->start) + "," + toString(member.range->end) + ">";
				}
				
				ComponentFieldParameters parameters(componentType, ComponentDataRetriever(data, member.name, member.displayName.isEmpty() ? member.name : member.displayName), member.defaultValue);
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
	auto& components = getEntityData().getComponents();
	
	for (size_t i = 0; i < components.size(); ++i) {
		if (components[i].first == name) {
			components[i].second = std::move(data);

			needToReloadUI = true;
			onEntityUpdated();
			return;
		}
	}

	// Not found, add it instead
	addComponent(name, std::move(data));
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

ISceneEditorWindow& EntityEditor::getSceneEditorWindow() const
{
	return *sceneEditor;
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
	auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight"), LocalisedString::fromUserString(text));
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
		field->getWidgetAs<UILabel>("fieldName")->setText(LocalisedString::fromUserString(parameters.data.getLabelName()));
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
			container->add(std::make_shared<UILabel>("", factory.getStyle("labelLight"), LocalisedString::fromHardcodedString("N/A")));
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
