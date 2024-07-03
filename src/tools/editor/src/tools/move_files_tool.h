#pragma once

#include <halley.hpp>
using namespace Halley;

class MoveFilesTool : public UIWidget {
public:
	MoveFilesTool(UIFactory& factory, UIFactory& editorFactory, Project& project, Vector<ConfigBreadCrumb> configBreadCrumbs);
	~MoveFilesTool() override;

	void onMakeUI() override;
	void update(Time t, bool moved) override;

private:
	enum class Stage {
		Idle,
		Monitoring,
		ComputingChanges,
		ConfirmChanges,
		Done
	};

	struct MovedFile {
		String from;
		String to;
		bool isDir;
	};

	using ChangeList = Vector<std::pair<Path, Bytes>>;
	using MovedFilesByType = HashMap<ImportAssetType, Vector<MovedFile>>;

	UIFactory& factory;
	UIFactory& editorFactory;
	Project& project;

	DirectoryMonitor monitor;
	Stage curStage = Stage::Idle;

	std::shared_ptr<UIList> log;

	Vector<MovedFile> filesMoved;
	std::optional<String> lastRemove;
	bool logDirty = false;
	Future<void> computing;

	ChangeList toChange;
	ChangeList toChangeOutbox;
	std::mutex toChangeMutex;

	Vector<ConfigBreadCrumb> configBreadCrumbs;
	HashMap<String, Vector<std::pair<String, ImportAssetType>>> componentReplacementTable;
	HashMap<String, Vector<std::pair<String, ImportAssetType>>> cometReplacementTable;

	void onNext();
	void setStage(Stage stage);

	void addRename(String oldName, String newName, bool isDir);
	void populateLog();

	void receiveChangedFiles();
	void addChangesToLog(const ChangeList& entries);

	void startComputingChanges();
	void doneComputingChanges();
	void computeChanges();
	void confirmChanges();

	void addChangedFile(Path filePath, String data);
	void addChangedFile(Path filePath, Bytes data);

	void updateGameplayConfig(const MovedFilesByType& movedFiles, const Path& path);
	void updatePrefabs(const MovedFilesByType& movedFiles, const Path& path);
	void updateScenes(const MovedFilesByType& movedFiles, const Path& path);
	void updateAudioEvents(const MovedFilesByType& movedFiles, const Path& path);
	void updateAudioObjects(const MovedFilesByType& movedFiles, const Path& path);
	void updateCometScripts(const MovedFilesByType& movedFiles, const Path& path);
	void updateUI(const MovedFilesByType& movedFiles, const Path& path);

	bool updatePrefab(const MovedFilesByType& movedFiles, Prefab& prefab);
	bool updateEntityData(const MovedFilesByType& movedFiles, EntityData& data);
	bool updateComponentData(const MovedFilesByType& movedFiles, const String& componentName, ConfigNode& componentData);
	bool updateConfigNode(const MovedFilesByType& movedFiles, ConfigNode& node, ImportAssetType type);
	std::optional<String> updateString(const Vector<MovedFile>& movedFiles, const String& str);

	bool updateUI(const MovedFilesByType& movedFiles, UIDefinition& ui);
	bool updateUINode(const MovedFilesByType& movedFiles, ConfigNode& uiNode);
	bool updateUIWidget(const MovedFilesByType& movedFiles, ConfigNode& widgetNode);

	bool updateAudioEvent(const MovedFilesByType& movedFiles, AudioEvent& event);
	bool updateAudioObject(const MovedFilesByType& movedFiles, AudioObject& object);
	void updateAudioObject(const Vector<MovedFile>& movedFiles, IAudioObject& object, bool& modified);

	bool updateCometScript(const MovedFilesByType& movedFiles, ScriptGraph& script);

	void setupComponentReplacementTable();
	void setupCometNodeReplacementTable();

	struct ConfigChange {
		int line;
		int column;
		String from;
		String to;
	};

	struct ConfigBreadCrumbRef {
		gsl::span<const String> breadCrumb;
		ImportAssetType assetType;

		ConfigBreadCrumbRef() = default;
		ConfigBreadCrumbRef(const ConfigBreadCrumb& crumb);
		ConfigBreadCrumbRef(gsl::span<const String> breadCrumb, ImportAssetType assetType);

		ConfigBreadCrumbRef dropFront() const;
		bool empty() const;
		bool startsWith(const String& value) const;
	};

	Vector<ConfigChange> findConfigChanges(const MovedFilesByType& movedFiles, const ConfigNode& config);
	void doFindConfigChanges(Vector<ConfigChange>& changes, const MovedFilesByType& movedFiles, const Vector<ConfigBreadCrumbRef>& breadCrumbs, const ConfigNode& config);
	void checkConfigChanges(Vector<ConfigChange>& changes, const MovedFilesByType& movedFiles, const ConfigNode& config, ImportAssetType type);
	void applyChangesToLines(const Vector<ConfigChange>& changes, Vector<String>& lines);
};
