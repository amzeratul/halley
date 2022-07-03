#include "script_graph_editor.h"

#include "script_gizmo_ui.h"
#include "halley/tools/project/project.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"
using namespace Halley;

ScriptGraphEditor::ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::ScriptGraph)
	, projectWindow(projectWindow)
{
}

ScriptGraphEditor::~ScriptGraphEditor()
{
	setListeningToClient(false);
	assert(!scriptEnumHandle);
	assert(!scriptStateHandle);
}

void ScriptGraphEditor::onActiveChanged(bool active)
{
	setListeningToClient(active);
}

void ScriptGraphEditor::onMakeUI()
{
	gizmoEditor = std::make_shared<ScriptGizmoUI>(factory, gameResources, *projectWindow.getEntityEditorFactory(), projectWindow.getScriptNodeTypes(), 
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

	bindData("instances", "-1", [=](String id)
	{
		setCurrentInstance(id.toInteger64());
	});
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
		infiniCanvas->setScrollEnabled(!gizmoEditor->isHighlighted());
	}
}

void ScriptGraphEditor::setListeningToClient(bool listening)
{
	auto& devConServer = *project.getDevConServer();

	if (listening) {
		if (!scriptEnumHandle) {
			onScriptEnum(ConfigNode::SequenceType{});
			scriptEnumHandle = devConServer.registerInterest("scriptEnum", ConfigNode(assetId), [=] (ConfigNode result)
			{
				onScriptEnum(std::move(result));
			});
		}
		setListeningToState(curEntityId);
	} else {
		if (scriptEnumHandle) {
			devConServer.unregisterInterest(scriptEnumHandle.value());
			scriptEnumHandle.reset();
			onScriptEnum(ConfigNode::SequenceType{});
		}
		setListeningToState(-1);
	}
}

void ScriptGraphEditor::setListeningToState(int64_t entityId)
{
	auto& devConServer = *project.getDevConServer();

	if (scriptStateHandle) {
		devConServer.unregisterInterest(scriptStateHandle.value());
		scriptStateHandle.reset();
	}

	if (entityId != -1) {
		ConfigNode::MapType params;
		params["entityId"] = curEntityId;
		params["scriptId"] = assetId;
		scriptStateHandle = devConServer.registerInterest("scriptState", params, [=] (ConfigNode result)
		{
			onScriptState(std::move(result));
		});
	}
}

void ScriptGraphEditor::onScriptEnum(ConfigNode data)
{
	const auto instances = getWidgetAs<UIDropdown>("instances");
	Vector<String> ids;
	Vector<LocalisedString> names;
	ids.push_back("-1");
	names.push_back(LocalisedString::fromHardcodedString("[none]"));

	for (const auto& entry: data.asSequence()) {
		ids.push_back(toString(entry["entityId"].asInt64()));
		names.push_back(LocalisedString::fromUserString(entry["name"].asString() + " (" + toString(entry["entityId"].asInt64()) + ")"));
	}

	instances->setOptions(std::move(ids), std::move(names));
}

void ScriptGraphEditor::onScriptState(ConfigNode data)
{
	YAMLConvert::EmitOptions options;
	Logger::logDev("Got script state:\n" + YAMLConvert::generateYAML(data, options));
}

void ScriptGraphEditor::setCurrentInstance(int64_t entityId)
{
	if (curEntityId != entityId) {
		curEntityId = entityId;
		if (scriptEnumHandle) {
			setListeningToState(entityId);
		}
	}
}
