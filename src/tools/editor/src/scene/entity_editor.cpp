#include "entity_editor.h"

#include "choose_asset_window.h"
#include "entity_editor_factories.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "scene_editor_window.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
{
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
	makeUI();
	reloadEntity();
}

void EntityEditor::update(Time t, bool moved)
{
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
}

void EntityEditor::makeUI()
{
	add(factory.makeUI("ui/halley/entity_editor"), 1);
	fields = getWidget("fields");
	fields->setMinSize(Vector2f(300, 20));

	entityName = getWidgetAs<UITextInput>("entityName");
	prefabName = getWidgetAs<UIDropdown>("prefabName");

	setHandle(UIEventType::ButtonClicked, "addComponentButton", [=](const UIEvent& event)
	{
		addComponent();
	});

	setHandle(UIEventType::TextSubmit, "entityName", [=] (const UIEvent& event)
	{
		setName(event.getStringData());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "prefabName", [=] (const UIEvent& event)
	{
		setPrefabName(event.getStringData());
	});
}

bool EntityEditor::loadEntity(const String& id, ConfigNode& data, const ConfigNode* prefab, bool force, Resources& resources)
{
	Expects(ecsData);

	gameResources = &resources;
	context = std::make_unique<ComponentEditorContext>(*static_cast<IEntityEditor*>(this), factory, resources);

	if (currentId == id && currentEntityData == &data && !force) {
		return false;
	}

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
			updatePrefabNames();
			prefabName->setSelectedOption(getEntityData()["prefab"].asString(""));
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
			if (member.serializable) {
				ComponentFieldParameters parameters(componentType, ComponentDataRetriever(data, member.name), member.defaultValue, componentNames);
				createField(*componentFields, member.type.name, parameters);
			}
		}
	}
	
	fields->add(componentUI);
}

void EntityEditor::createField(UIWidget& parent, const String& fieldType, const ComponentFieldParameters& parameters)
{
	const auto iter = fieldFactories.find(fieldType);
	auto* compFieldFactory = iter != fieldFactories.end() ? iter->second.get() : nullptr;
		
	if (compFieldFactory && compFieldFactory->canCreateLabel()) {
		compFieldFactory->createLabelAndField(parent, *context, parameters);
	} else if (compFieldFactory && compFieldFactory->isCompound()) {
		auto field = factory.makeUI("ui/halley/entity_editor_compound_field");
		field->getWidgetAs<UILabel>("fieldName")->setText(LocalisedString::fromUserString(parameters.data.getName()));
		field->getWidget("fields")->add(compFieldFactory->createField(*context, parameters));
		parent.add(field);
	} else {
		auto container = std::make_shared<UISizer>();
		container->add(makeLabel(parameters.data.getName()), 0, {}, UISizerAlignFlags::CentreVertical);

		if (compFieldFactory) {
			container->add(compFieldFactory->createField(*context, parameters), 1);
		} else {
			container->add(std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromHardcodedString("N/A")));
		}

		parent.add(container);
	}
}

void EntityEditor::addComponent()
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

	// Components on prefab root
	std::set<String> prefabComponents;
	if (isPrefab) {
		auto& comps = (*prefabData)["components"];
		if (comps.getType() == ConfigNodeType::Sequence) {
			for (auto& pc: comps.asSequence()) {
				for (auto& kv: pc.asMap()) {
					prefabComponents.insert(kv.first);
				}
			}
		}
	}

	// Generate all available names
	std::vector<String> componentNames;
	for (auto& c: ecsData->getComponents()) {
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
	auto& components = getEntityData()["components"];
	if (components.getType() != ConfigNodeType::Sequence) {
		components = ConfigNode::SequenceType();
	}

	ConfigNode compNode = ConfigNode::MapType();
	compNode[name] = ConfigNode::MapType();
	
	components.asSequence().emplace_back(std::move(compNode));

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
	if (!isPrefab) {
		getEntityData()["name"] = name;
		onEntityUpdated();
	}
}

void EntityEditor::setPrefabName(const String& name)
{
	if (isPrefab) {
		getEntityData()["prefab"] = name;
		onEntityUpdated();
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

void EntityEditor::updatePrefabNames()
{
	Expects(prefabName);

	prefabName->setOptions(gameResources->enumerate<Prefab>());
}

void EntityEditor::addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}
