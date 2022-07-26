#include "script_graph_editor.h"

#include "script_gizmo_ui.h"
#include "halley/tools/project/project.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"
using namespace Halley;

ScriptGraphEditor::ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::ScriptGraph)
	, projectWindow(projectWindow)
	, gameResources(gameResources)
{
}

ScriptGraphEditor::~ScriptGraphEditor()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (dllListenerAdded) {
			dll.removeReloadListener(*this);
		}
	});

	setListeningToClient(false);
	assert(!scriptEnumHandle);
	assert(!scriptStateHandle);
}

void ScriptGraphEditor::onActiveChanged(bool active)
{
	setListeningToClient(active);
	if (active && gizmoEditor) {
		gizmoEditor->updateNodes();
	}
}

void ScriptGraphEditor::onMakeUI()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (!dllListenerAdded) {
			dll.addReloadListener(*this);
			dllListenerAdded = true;
		}
	});

	scriptNodeTypes = projectWindow.getScriptNodeTypes();
	entityEditorFactory = std::make_shared<EntityEditorFactory>(projectWindow.getEntityEditorFactoryRoot(), nullptr);

	gizmoEditor = std::make_shared<ScriptGizmoUI>(factory, gameResources, *entityEditorFactory, scriptNodeTypes, 
		projectWindow.getAPI().input->getKeyboard(), projectWindow.getAPI().system->getClipboard(), [=] ()
	{
		markModified();
	});
	
	if (scriptGraph) {
		gizmoEditor->load(*scriptGraph);
	}

	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->clear();
	infiniCanvas->add(gizmoEditor, 0, {}, UISizerAlignFlags::Centre, Vector2f());
	infiniCanvas->setMouseMirror(gizmoEditor);
	infiniCanvas->setZoomEnabled(false);
	infiniCanvas->setLeftClickScrollKey(KeyCode::Space);

	infiniCanvas->setZoomListener([=] (float zoom)
	{
		gizmoEditor->setZoom(zoom);
	});

	getWidget("toolbarGizmo")->clear();
	getWidget("toolbarGizmo")->add(gizmoEditor->makeUI());

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

	const auto assetKey = toString(assetType) + ":" + assetId;
	autoAcquire = projectWindow.getAssetSetting(assetKey, "autoAcquire").asBool(true);
	bindData("autoAcquire", autoAcquire, [=](bool value)
	{
		projectWindow.setAssetSetting(assetKey, "autoAcquire", ConfigNode(value));
		autoAcquire = value;
		tryAutoAcquire();
	});
	tryAutoAcquire();
}

void ScriptGraphEditor::reload()
{
}

void ScriptGraphEditor::refreshAssets()
{
}

void ScriptGraphEditor::save()
{
	if (modified) {
		modified = false;

		const auto assetPath = Path("comet/" + scriptGraph->getAssetId() + ".comet");
		const auto strData = scriptGraph->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);
	}
}

bool ScriptGraphEditor::isModified()
{
	return modified;
}

void ScriptGraphEditor::markModified()
{
	modified = true;
}

void ScriptGraphEditor::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Unloaded) {
		clear();
		gizmoEditor.reset();
		infiniCanvas.reset();
		scriptNodeTypes.reset();
		entityEditorFactory.reset();
		needsLoading = true;
		hasUI = false;
	}
}

std::shared_ptr<const Resource> ScriptGraphEditor::loadResource(const String& assetId)
{
	if (project.isDLLLoaded()) {
		open();
	} else {
		pendingLoad = true;
	}
	
	return {};
}

void ScriptGraphEditor::open()
{
	Expects (project.isDLLLoaded());

	if (!hasUI) {
		factory.loadUI(*this, "halley/script_graph_editor");
		hasUI = true;
	}
	
	const auto assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(assetType, assetId);
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);

	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		scriptGraph = std::make_shared<ScriptGraph>(config.getRoot());
	} else {
		scriptGraph = std::make_shared<ScriptGraph>();
		scriptGraph->makeDefault();
		markModified();
	}
	scriptGraph->setAssetId(assetId);

	if (gizmoEditor) {
		gizmoEditor->load(*scriptGraph);
	}

	setListeningToClient(true);
}

void ScriptGraphEditor::update(Time time, bool moved)
{
	if (pendingLoad && project.isDLLLoaded()) {
		pendingLoad = false;
		open();
	}

	AssetEditor::update(time, moved);

	if (gizmoEditor) {
		gizmoEditor->setState(scriptState.get());
		infiniCanvas->setScrollEnabled(!gizmoEditor->isHighlighted());

		updateNodeUnderCursor();
	}
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
	auto& devConServer = *project.getDevConServer();

	if (listening) {
		if (!scriptEnumHandle) {
			refreshScriptEnum();
			scriptEnumHandle = devConServer.registerInterest("scriptEnum", ConfigNode(assetId), [=] (size_t connId, ConfigNode result)
			{
				onScriptEnum(connId, std::move(result));
			});
		}
		setListeningToState(curEntityId.value_or(std::pair<size_t, int64_t>(0, -1)));
	} else {
		if (scriptEnumHandle) {
			devConServer.unregisterInterest(scriptEnumHandle.value());
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
		params["entityId"] = entityId.second;
		params["scriptId"] = assetId;
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
		if (gizmoEditor) {
			gizmoEditor->setState(nullptr);
		}
	} else {
		if (!scriptState) {
			scriptState = std::make_unique<ScriptState>();
		}

		scriptGraph->assignTypes(*scriptNodeTypes);

		EntitySerializationContext context;
		context.resources = &gameResources;
		scriptState->load(data["scriptState"], context);
		scriptState->setScriptGraphPtr(scriptGraph.get());
		scriptState->prepareStates(context, 0);

		onCurNodeData(data["curNode"]);
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

void ScriptGraphEditor::onScriptEnum(size_t connId, ConfigNode data)
{
	std_ex::erase_if(curEntities, [&](const auto& e) { return e.connId == connId; });

	if (data.getType() == ConfigNodeType::Sequence) {
		for (const auto& entry: data.asSequence()) {
			curEntities.emplace_back(EntityEnumData{ connId, entry["entityId"].asInt64(), entry["name"].asString() });
		}
	}

	if (curEntityId && curEntityId->first == connId && !std_ex::contains_if(curEntities, [&] (const EntityEnumData& d) { return d.entityId == curEntityId->second && d.connId == curEntityId->first; })) {
		curEntityId.reset();
		if (!tryAutoAcquire()) {
			getWidgetAs<UIDropdown>("instances")->setSelectedOption(0);
		}
	}

	refreshScriptEnum();
}

void ScriptGraphEditor::refreshScriptEnum()
{
	if (!hasUI) {
		return;
	}
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
