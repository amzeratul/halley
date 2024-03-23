#include "script_graph_editor.h"
#include "script_gizmo_ui.h"
#include "script_target_entity_factory.h"
#include "halley/tools/project/project.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"
#include "src/assets/graph/script_graph_variable_inspector.h"

using namespace Halley;

ScriptGraphEditor::ScriptGraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow,
	std::shared_ptr<ScriptGraph> scriptGraph, AssetEditor* assetEditor, std::shared_ptr<const Scene> scene, Callback callback, Vector<String> entityTargets)
	: GraphEditor(factory, gameResources, projectWindow, scriptGraph, assetEditor, scene, callback)
	, scriptGraph(std::move(scriptGraph))
	, entityTargets(std::move(entityTargets))
{
}

ScriptGraphEditor::~ScriptGraphEditor()
{
	setListeningToClient(false);
	assert(!scriptEnumHandle);
	assert(!scriptStateHandle);
}

void ScriptGraphEditor::init()
{
	GraphEditor::init();
	setListeningToClient(true);
}

void ScriptGraphEditor::onMakeUI()
{
	scriptNodeTypes = projectWindow.getScriptNodeTypes();
	entityEditorFactory = std::make_shared<EntityEditorFactory>(projectWindow.getEntityEditorFactoryRoot(), nullptr);
	if (callback) {
		// Only add this overriding default factory if we're running inside scene editor
		entityEditorFactory->addFieldFactory(std::make_unique<ScriptTargetEntityFactory>(*this));

		if (scene) {
			for (auto& f: project.getGameInstance()->createCustomScriptEditorFieldFactories(*scene, gameResources)) {
				entityEditorFactory->addFieldFactory(std::move(f));
			}
		}
	}

	gizmoEditor = std::make_shared<ScriptGizmoUI>(factory, gameResources, *entityEditorFactory, scriptNodeTypes, 
		projectWindow.getAPI().input->getKeyboard(), projectWindow.getAPI().system->getClipboard(), *this
	);
	gizmoEditor->setEntityTargets(entityTargets);
	
	if (scriptGraph) {
		gizmoEditor->load(*scriptGraph);
	}
	const auto assetKey = scriptGraph ? (toString(AssetType::ScriptGraph) + ":" + scriptGraph->getAssetId()) : "";

	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->clear();
	infiniCanvas->add(gizmoEditor, 0, {}, UISizerAlignFlags::Top | UISizerAlignFlags::Left);
	infiniCanvas->setMouseMirror(gizmoEditor);
	infiniCanvas->setZoomEnabled(false);
	infiniCanvas->setLeftClickScrollKey(KeyCode::Space);

	infiniCanvas->setZoomListener([=] (float zoom)
	{
		gizmoEditor->setZoom(zoom);
	});
	infiniCanvas->setScrollPosition(projectWindow.getAssetSetting(assetKey, "position").asVector2f({}));
	infiniCanvas->setScrollListener([=] (Vector2f pos)
	{
		projectWindow.setAssetSetting(assetKey, "position", ConfigNode(pos));
	});

	variableInspector = getWidgetAs<ScriptGraphVariableInspector>("ScriptGraphVariableInspector");

	getWidget("toolbarGizmo")->clear();
	getWidget("toolbarGizmo")->add(gizmoEditor->makeUI());

	getWidget("drillUpBar")->setActive(!!callback);

	bindData("instances", "-1:-1", [=](String id)
	{
		auto split = id.split(':');
		if (split.size() == 2 && split[0].isInteger() && split[1].isInteger()) {
			auto connId = split[0].toInteger();
			auto entityId = split[1].toInteger64();
			setCurrentInstance({ connId, entityId });
		} else {
			setCurrentInstance({ 0, -1 });
		}
	});

	autoAcquire = projectWindow.getAssetSetting(assetKey, "autoAcquire").asBool(true);
	bindData("autoAcquire", autoAcquire, [=](bool value)
	{
		projectWindow.setAssetSetting(assetKey, "autoAcquire", ConfigNode(value));
		autoAcquire = value;
		tryAutoAcquire();
	});
	tryAutoAcquire();

	const auto autoConnect = projectWindow.getSetting(EditorSettingType::Project, "autoConnectPins").asBool(true);
	gizmoEditor->setAutoConnectPins(autoConnect);
	bindData("autoConnectPins", autoConnect, [=](bool value)
	{
		projectWindow.setSetting(EditorSettingType::Project, "autoConnectPins", ConfigNode(value));
		gizmoEditor->setAutoConnectPins(value);
	});

	variableInspectorEnabled = projectWindow.getSetting(EditorSettingType::Project, "variableInspectorEnabled").asBool(true);
	bindData("variableInspectorEnabled", variableInspectorEnabled, [=](bool value)
	{
		projectWindow.setSetting(EditorSettingType::Project, "variableInspectorEnabled", ConfigNode(value));
		variableInspectorEnabled = value;
	});

	setHandle(UIEventType::ButtonClicked, "ok", [=](const UIEvent& event)
	{
		callback(true, scriptGraph);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		callback(false, scriptGraph);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "undoButton", [=](const UIEvent& event)
	{
		undo();
	});

	setHandle(UIEventType::ButtonClicked, "redoButton", [=](const UIEvent& event)
	{
		redo();
	});

	setHandle(UIEventType::ButtonClicked, "centreViewButton", [=](const UIEvent& event)
	{
		centreView();
	});

	setHandle(UIEventType::ButtonClicked, "propertiesButton", [=](const UIEvent& event)
	{
		openProperties();
	});

	getWidget("saveButton")->setActive(assetEditor);
	if (assetEditor) {
		setHandle(UIEventType::ButtonClicked, "saveButton", [=](const UIEvent& event)
		{
			assetEditor->save();
		});
	}
}

void ScriptGraphEditor::setScriptGraph(std::shared_ptr<ScriptGraph> graph)
{
	scriptGraph = std::move(graph);
	setGraph(scriptGraph);
}

void ScriptGraphEditor::onActiveChanged(bool active)
{
	setListeningToClient(active);
	if (active && gizmoEditor) {
		gizmoEditor->updateNodes();
	}
}

void ScriptGraphEditor::setModified(bool value)
{
	modified = value;

	if (value) {
		scriptGraph->updateHash();
		if (scriptEnumHandle) {
			setListeningToClient(false);
			setListeningToClient(true);
		}
	}
}

bool ScriptGraphEditor::isModified()
{
	return modified;
}

void ScriptGraphEditor::drillDownSave()
{
	callback(true, scriptGraph);
	setModified(false);
}

std::shared_ptr<ScriptGraph> ScriptGraphEditor::getScriptGraph()
{
	return scriptGraph;
}

const Vector<String>& ScriptGraphEditor::getScriptTargetIds() const
{
	return entityTargets;
}

std::shared_ptr<const Scene> ScriptGraphEditor::getScene() const
{
	return scene;
}

void ScriptGraphEditor::onModified()
{
	setModified(true);
	if (scriptGraph) {
		undoStack.update(scriptGraph->toConfigNode());
	}
}

void ScriptGraphEditor::undo()
{
	if (scriptGraph && undoStack.canUndo()) {
		const auto assetId = scriptGraph->getAssetId();
		*scriptGraph = ScriptGraph(undoStack.undo());
		scriptGraph->setAssetId(assetId);
	}
}

void ScriptGraphEditor::redo()
{
	if (scriptGraph && undoStack.canRedo()) {
		const auto assetId = scriptGraph->getAssetId();
		*scriptGraph = ScriptGraph(undoStack.redo());
		scriptGraph->setAssetId(assetId);
	}
}

void ScriptGraphEditor::centreView()
{
	infiniCanvas->setScrollPosition({});
}

void ScriptGraphEditor::openProperties()
{
	if (scriptGraph) {
		getRoot()->addChild(std::make_shared<ScriptGraphProperties>(factory, *scriptGraph, [=] ()
		{
			onModified();
		}));
	}
}

std::shared_ptr<UIWidget> ScriptGraphEditor::asWidget()
{
	return shared_from_this();
}

void ScriptGraphEditor::update(Time t, bool moved)
{
	if (gizmoEditor) {
		gizmoEditor->setState(scriptState.get());
		infiniCanvas->setScrollEnabled(!gizmoEditor->isHighlighted());

		updateNodeUnderCursor();
	}

	if (assetEditor) {
		getWidget("saveButton")->setEnabled(assetEditor->isModified());
	}
	getWidget("undoButton")->setEnabled(undoStack.canUndo());
	getWidget("redoButton")->setEnabled(undoStack.canRedo());
}

void ScriptGraphEditor::setCurrentInstance(std::pair<size_t, int64_t> entityId)
{
	if (curEntityId != entityId) {
		if (entityId.second == -1) {
			curEntityId.reset();
			scriptState.reset();
			if (gizmoEditor) {
				gizmoEditor->setState(nullptr);
			}
		} else {
			curEntityId = entityId;
		}

		if (scriptEnumHandle) {
			setListeningToState(entityId);
		}
	}
}

void ScriptGraphEditor::setListeningToClient(bool listening)
{
	auto* devConServer = project.getDevConServer();
	if (!devConServer) {
		return;
	}

	if (listening) {
		refreshScriptEnum();
		ConfigNode::MapType params;
		scriptGraph->updateHash();
		params["scriptId"] = scriptGraph->getAssetId();
		params["scriptHash"] = static_cast<int64_t>(scriptGraph->getAssetHash());
		scriptEnumHandle = devConServer->registerInterest("scriptEnum", std::move(params), [=] (size_t connId, ConfigNode result)
		{
			onScriptEnum(connId, std::move(result));
		});
		setListeningToState(curEntityId.value_or(std::pair<size_t, int64_t>(0, -1)));
	} else {
		if (scriptEnumHandle) {
			devConServer->unregisterInterest(scriptEnumHandle.value());
			scriptEnumHandle.reset();
			curEntities.clear();
			refreshScriptEnum();
		}
		entityIdBeforeSuspend = curEntityId;
		setListeningToState({0, -1});
		curEntityId.reset();
	}
}

void ScriptGraphEditor::setListeningToState(std::pair<size_t, int64_t> entityId)
{
	auto& devConServer = *project.getDevConServer();

	if (scriptStateHandle) {
		devConServer.unregisterInterest(scriptStateHandle.value());
		scriptStateHandle.reset();
	}

	if (entityId.second != -1) {
		ConfigNode::MapType params;
		params["connId"] = static_cast<int>(entityId.first);
		params["entityId"] = EntityIdHolder{ entityId.second };
		params["scriptId"] = scriptGraph->getAssetId();
		params["scriptHash"] = static_cast<int64_t>(scriptGraph->getAssetHash());
		params["curNode"] = getCurrentNodeConfig();
		scriptStateHandle = devConServer.registerInterest("scriptState", params, [=] (size_t connId, ConfigNode result)
		{
			onScriptState(connId, std::move(result));
		});
	} else {
		onScriptState(curEntityId ? curEntityId->first : 0, ConfigNode());
	}
}

void ScriptGraphEditor::updateNodeUnderCursor()
{
	if (scriptStateHandle) {
		auto& devConServer = *project.getDevConServer();
		auto params = ConfigNode(devConServer.getInterestParams(scriptStateHandle.value()));
		params["curNode"] = getCurrentNodeConfig();
		devConServer.updateInterest(scriptStateHandle.value(), std::move(params));
	}
}

ConfigNode ScriptGraphEditor::getCurrentNodeConfig()
{
	if (gizmoEditor) {
		if (const auto node = gizmoEditor->getNodeUnderMouse()) {
			ConfigNode::MapType result;
			result["nodeId"] = node->nodeId;
			result["elementId"] = static_cast<int>(static_cast<int8_t>(node->elementId));
			return result;
		}
	}

	return {};
}

void ScriptGraphEditor::onScriptState(size_t connId, ConfigNode data)
{
	if (!curEntityId || curEntityId->first != connId) {
		return;
	}

	if (data.getType() == ConfigNodeType::Undefined) {
		scriptState.reset();
		scriptGraph->setRoots({});
		if (gizmoEditor) {
			gizmoEditor->setState(nullptr);
		}
		onDebugDisplayData({});
		variableInspector->updateVariables(ConfigNode());
	} else {
		if (!scriptState) {
			scriptState = std::make_unique<ScriptState>();
		}

		scriptGraph->assignTypes(*scriptNodeTypes);

		const auto nodeRange = data["nodeRange"].asVector2i(Vector2i(0, std::numeric_limits<GraphNodeId>::max()));

		EntitySerializationContext context;
		context.resources = &gameResources;
		scriptState->load(data["scriptState"], context);
		scriptState->setScriptGraphPtr(scriptGraph.get());
		scriptState->offsetToNodeRange(Range<GraphNodeId>(nodeRange.x, nodeRange.y));
		scriptState->prepareStates(context, 0);

		scriptGraph->setRoots(ScriptGraphNodeRoots(data["roots"]));

		onCurNodeData(data["curNode"]);
		onDebugDisplayData(data["debugDisplays"]);

		const auto emptyNode = ConfigNode();
		variableInspector->updateVariables(variableInspectorEnabled ? data["variables"] : emptyNode);
	}
}

void ScriptGraphEditor::onCurNodeData(const ConfigNode& curNodeData)
{
	const auto requested = getCurrentNodeConfig();
	if (requested.getType() != ConfigNodeType::Undefined && curNodeData.getType() != ConfigNodeType::Undefined && requested["nodeId"] == curNodeData["nodeId"] && requested["elementId"] == curNodeData["elementId"]) {
		setCurNodeData(curNodeData["value"].asString(""));
	} else {
		setCurNodeData("");
	}
}

void ScriptGraphEditor::setCurNodeData(const String& str)
{
	if (gizmoEditor) {
		gizmoEditor->setCurNodeDevConData(str);
	}
}

void ScriptGraphEditor::onDebugDisplayData(const ConfigNode& node)
{
	HashMap<int, String> values;
	if (node.getType() == ConfigNodeType::Sequence) {
		for (const auto& n: node.asSequence()) {
			auto val = n["value"].asString();
			if (n["value"].getType() == ConfigNodeType::String) {
				val = "\"" + val + "\"";
			}
			values[n["nodeId"].asInt()] = std::move(val);
		}
	}
	if (gizmoEditor) {
		gizmoEditor->setDebugDisplayData(std::move(values));
	}
}

void ScriptGraphEditor::onScriptEnum(size_t connId, ConfigNode data)
{
	std_ex::erase_if(curEntities, [&](const auto& e) { return e.connId == connId; });

	if (data.getType() == ConfigNodeType::Sequence) {
		for (const auto& entry: data.asSequence()) {
			curEntities.emplace_back(EntityEnumData{ connId, entry["entityId"].asEntityId().value, entry["name"].asString(), entry["scriptIdx"].asInt() });
		}
	}

	if (curEntityId && curEntityId->first == connId && !std_ex::contains_if(curEntities, [&] (const EntityEnumData& d) { return d.entityId == curEntityId->second && d.connId == curEntityId->first; })) {
		curEntityId.reset();
		if (!tryAutoAcquire()) {
			getWidgetAs<UIDropdown>("instances")->setSelectedOption(0);
		}
	}

	if (curEntities.empty()) {
		onDebugDisplayData({});
		variableInspector->updateVariables(ConfigNode());
	}

	refreshScriptEnum();
}

void ScriptGraphEditor::refreshScriptEnum()
{
	const auto instances = getWidgetAs<UIDropdown>("instances");
	Vector<String> ids;
	Vector<LocalisedString> names;
	ids.push_back("-1:-1");
	names.push_back(LocalisedString::fromHardcodedString("[none]"));

	for (const auto& e: curEntities) {
		ids.push_back(toString(e.connId) + ":" + toString(e.entityId)); 
		names.push_back(LocalisedString::fromUserString("[" + toString(e.connId) + "] " + e.name + " (" + toString(e.entityId) + ")"));
	}

	instances->setOptions(std::move(ids), std::move(names));

	tryAutoAcquire();
}

bool ScriptGraphEditor::tryAutoAcquire()
{
	if (autoAcquire && !curEntityId) {
		if (!curEntities.empty()) {
			size_t bestIdx = 0;
			if (entityIdBeforeSuspend) {
				const auto iter = std_ex::find_if(curEntities, [&](const EntityEnumData& e) { return e.connId == entityIdBeforeSuspend->first && e.entityId == entityIdBeforeSuspend->second; });
				if (iter != curEntities.end()) {
					bestIdx = iter - curEntities.begin();
				}
			}

			const auto id = toString(curEntities[bestIdx].connId) + ":" + toString(curEntities[bestIdx].entityId);
			const auto instances = getWidgetAs<UIDropdown>("instances");
			instances->setSelectedOption(id);
			return true;
		}
	}

	return false;
}

ScriptGraphProperties::ScriptGraphProperties(UIFactory& factory, ScriptGraph& script, Callback callback)
	: PopupWindow("scriptGraphProperties")
	, factory(factory)
	, scriptGraph(script)
	, callback(std::move(callback))
	, properties(script.getProperties())
{
	factory.loadUI(*this, "halley/script_graph_properties");
}

void ScriptGraphProperties::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		scriptGraph.getProperties() = std::move(properties);
		callback();
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		destroy();
	});

	bindData("persistent", scriptGraph.isPersistent(), [=] (bool value)
	{
		properties["persistent"] = value;
	});

	bindData("multiCopy", scriptGraph.isMultiCopy(), [=] (bool value)
	{
		properties["multiCopy"] = value;
	});

	bindData("supressDuplicateWarning", scriptGraph.isSupressDuplicateWarning(), [=] (bool value)
	{
		properties["supressDuplicateWarning"] = value;
	});

	bindData("network", scriptGraph.isNetwork(), [=] (bool value)
	{
		properties["network"] = value;
	});
}
