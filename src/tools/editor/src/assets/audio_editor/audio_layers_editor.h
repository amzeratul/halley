#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;
	class AudioSubObjectLayers;

	class AudioLayersEditor : public UIWidget {
    public:
        AudioLayersEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectLayers& layers);

	private:
		UIFactory& factory;
        AudioObjectEditor& editor;
		AudioSubObjectLayers& layers;
    };
}
