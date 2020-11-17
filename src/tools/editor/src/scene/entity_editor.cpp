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
	resetFieldFactories();
	makeUI();
	reloadEntity();
}

void EntityEditor::onAddedToRoot()
{
	getRoot()->registerKeyPressListener(shared_from_this());
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
}

bool EntityEditor::loadEntity(const String& id, ConfigNode& data, const Prefab* prefab, bool force, Resources& resources)
{
	Expects(ecsData);

	gameResources = &resources;
	prefabName->setGameResources(*gameResources);

	if (currentId == id && currentEntityData == &data && !force) {
		return false;
	}

	context = std::make_unique<ComponentEditorContext>(*static_cast<IEntityEditor*>(this), factory, resources);
	currentEntityData = &data;
	prefabData = prefab;
	currentId = id;
	isPrefab = !!prefabData;

	reloadEntity();
	return true;
}

void EntityEditor::unloadEntity()
{
	currentEntityData = nullptr;
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

	if (currentEntityData) {
		if (getEntityData()["components"].getType() == ConfigNodeType::Sequence) {
			auto& seq = getEntityData()["components"].asSequence();
			std::vector<String> componentNames;
			componentNames.reserve(seq.size());

			for (auto& componentNode: seq) {
				for (auto& c: componentNode.asMap()) {
					componentNames.push_back(c.first);
				}
			}
			
			for (auto& componentNode: seq) {
				for (auto& c: componentNode.asMap()) {
					loadComponentData(c.first, c.second, componentNames);
				}
			}
		}

		if (isPrefab) {
			prefabName->setValue(getEntityData()["prefab"].asString(""));
		} else {
			entityName->setText(getEntityData()["name"].asString(""));
		}
	}
}

void EntityEditor::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	sendEventDown(UIEvent(UIEventType::ReloadData, componentName + ":" + fieldName));
}

std::shared_ptr<IUIElement> EntityEditor::makeLabel(const String& text)
{
	auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromUserString(text));
	label->setMaxWidth(100);
	label->setMarquee(true);
	auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
	labelBox->add(label);
	return labelBox;
}

void EntityEditor::loadComponentData(const String& componentType, ConfigNode& data, const std::vector<String>& componentNames)
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
				ComponentFieldParameters parameters(componentType, componentNames, ComponentDataRetriever(data, member.name), member.defaultValue);
				auto field = makeField(member.type.name, parameters, member.collapse ? ComponentEditorLabelCreation::Never : ComponentEditorLabelCreation::Always);
				if (field) {
					componentFields->add(field);
				}
			}
		}
	}
	
	fields->add(componentUI);
}

std::pair<String, std::vector<String>> EntityEditor::parseType(const String& type)
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

std::shared_ptr<IUIElement> EntityEditor::makeField(const String& rawFieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel)
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
			container->add(compFieldFactory->createField(*context, parameters), 1);
		} else {
			container->add(std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromHardcodedString("N/A")));
		}

		return container;
	}

	return {};
}

ConfigNode EntityEditor::getDefaultNode(const String& fieldType)
{
	const auto iter = fieldFactories.find(fieldType);
	if (iter == fieldFactories.end()) {
		return ConfigNode();
	} else {
		return iter->second->getDefaultNode();
	}
}

std::set<String> EntityEditor::getComponentsOnEntity() const
{
	// Components already on this entity
	std::set<String> existingComponents;
	auto& components = getEntityData()["components"];
	if (components.getType() == ConfigNodeType::Sequence) {
		for (auto& c: components.asSequence()) {
			for (auto& kv: c.asMap()) {
				existingComponents.insert(kv.first);
			}
		}
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

	// Generate component
	ConfigNode compNode = ConfigNode::MapType();
	compNode[name] = ConfigNode::MapType();

	// Get list
	auto& components = getEntityData()["components"];
	components.ensureType(ConfigNodeType::Sequence);

	// Insert dependencies, if needed
	auto existingComponents = getComponentsOnEntity();
	auto prefabComponents = getComponentsOnPrefab();
	for (const auto& dep: deps) {
		if (existingComponents.find(dep) == existingComponents.end() && prefabComponents.find(dep) == prefabComponents.end()) {
			addComponent(dep);
		}
	}
	
	// Insert
	components.asSequence().emplace_back(std::move(compNode));

	// Reload
	needToReloadUI = true;	
	onEntityUpdated();
}

void EntityEditor::deleteComponent(const String& name)
{
	auto& components = getEntityData()["components"];
	if (components.getType() == ConfigNodeType::Sequence) {
		auto& componentSequence = components.asSequence();
		bool found = false;
		
		for (size_t i = 0; i < componentSequence.size(); ++i) {
			for (auto& c: componentSequence[i].asMap()) {
				if (c.first == name) {
					found = true;
					break;
				}
			}

			if (found) {
				componentSequence.erase(componentSequence.begin() + i);

				needToReloadUI = true;
				sceneEditor->onComponentRemoved(name);
				onEntityUpdated();
				return;
			}
		}
	}
}

void EntityEditor::setName(const String& name)
{
	if (!isPrefab && getName() != name) {
		getEntityData()["name"] = name;
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
	return isPrefab ? "" : getEntityData()["name"].asString("");
}

void EntityEditor::setPrefabName(const String& prefab)
{
	if (isPrefab) {
		const String oldPrefab = getEntityData()["prefab"].asString("");
		if (oldPrefab != prefab) {
			getEntityData()["prefab"] = prefab;
			onEntityUpdated();
		}
	}
}

void EntityEditor::editPrefab()
{
	if (isPrefab) {
		const String prefab = getEntityData()["prefab"].asString("");
		sceneEditor->openEditPrefabWindow(prefab);
	}
}

void EntityEditor::onEntityUpdated()
{
	sceneEditor->onEntityModified(currentId);
}

void EntityEditor::setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options)
{
	sceneEditor->setTool(tool, componentName, fieldName, std::move(options));
}

ConfigNode& EntityEditor::getEntityData()
{
	return *currentEntityData;
}

const ConfigNode& EntityEditor::getEntityData() const
{
	return *currentEntityData;
}

void EntityEditor::addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}

void EntityEditor::resetFieldFactories()
{
	fieldFactories.clear();
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
}
