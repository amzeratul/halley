#pragma once

#include "prec.h"
#include "halley/tools/project/project_loader.h"

namespace Halley
{
	class SystemAPI;

	class Preferences
	{
	public:
		Preferences();
		void setEditorVersion(String editorVersion);

		void loadDefaults();
		ConfigNode save() const;
		void load(const ConfigNode& config);

		bool isDirty() const;
		void saveToFile(SystemAPI& system) const;
		void loadFromFile(SystemAPI& system);

		void addRecent(Path path);
		const Vector<String>& getRecents() const;

		WindowDefinition getWindowDefinition() const;
		void updateWindowDefinition(const Window& window);

		const Vector<String>& getDisabledPlatforms() const;
		bool isPlatformDisabled(const String& name) const;
		void setPlatformDisabled(const String& name, bool disabled);

		const String& getColourScheme() const;
		void setColourScheme(String colourScheme);

		bool isLZ4HCEnabled() const;
		void setLZ4HCEnabled(bool enabled);
		bool isAutoBuild() const;
		void setAutoBuild(bool enabled);
		bool getCanEditHalleyAssets() const;
		void setCanEditHalleyAssets(bool enabled);

		void loadEditorPreferences(const Preferences& preferences);

		void applyProjectLoaderPreferences(ProjectLoader& projectLoader);

	private:
		mutable bool dirty = false;

		Vector<String> recents;
		String editorVersion;

		std::optional<Vector2i> windowPosition;
		Vector2i windowSize;
		WindowState windowState = WindowState::Normal;

		Vector<String> disabledPlatforms;

		String colourScheme;
		bool lz4hc = false;
		bool autoBuild = false;
		bool canEditHalleyAssets = false;
	};
}
