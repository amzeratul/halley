#pragma once

#include <halley.hpp>

#include "localisation_set_filters_window.h"

namespace Halley {
	class LocalisationExportWindow : public UIWidget {
	public:
		using Callback = std::function<void(bool, LocalisationExportOptions)>;
		LocalisationExportWindow(UIFactory& factory, Vector<String> chunks, Callback callback);

		void onMakeUI() override;
		void update(Time t, bool moved) override;

	private:
		UIFactory& factory;
		LocalisationFilters filters;
		Callback callback;

		Vector<String> chunks;
		HashSet<String> chunksToExport;
		LocalisationFiltersController filterController;

		bool acceptChunkUpdates = true;
		bool includeBOM = false;

		void populateChunkList();
		void setAllChunks(bool enabled);
		void setChunkEnabled(const String& chunkId, bool enabled);
		void onExport();
	};
}
