#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioSubObjectLayers;

	class AudioLayersEditor : public UIWidget {
    public:
        AudioLayersEditor(UIFactory& factory, AudioSubObjectLayers& layers);

	private:
		UIFactory& factory;
		AudioSubObjectLayers& layers;
    };
}
