#pragma once

#include "prec.h"

namespace Halley
{
	class SystemAPI;

	class Preferences
	{
	public:
		explicit Preferences(SystemAPI& system);

		ConfigNode save() const;
		void load(const ConfigNode& config);

		bool isDirty() const;
		void saveToFile() const;
		void loadFromFile();

		void addRecent(Path path);
		const std::vector<String>& getRecents() const;

		WindowDefinition getWindowDefinition() const;
		void updateWindowDefinition(const Window& window);

	private:
		SystemAPI& system;

		mutable bool dirty = false;

		std::vector<String> recents;

		Maybe<Vector2i> windowPosition;
		Vector2i windowSize;
		WindowState windowState = WindowState::Normal;
	};
}
