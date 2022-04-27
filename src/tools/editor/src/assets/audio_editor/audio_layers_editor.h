#pragma once

#include "halley/audio/sub_objects/audio_sub_object_layers.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;
	class AudioSubObjectLayers;

	class AudioLayersEditor : public UIWidget {
    public:
        AudioLayersEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectLayers& layers);

		void onMakeUI() override;

		AudioSubObjectLayers::Layer& getLayer(size_t idx);
		void markModified(size_t idx);
        AudioObjectEditor& getEditor();

    private:
		UIFactory& factory;
        AudioObjectEditor& editor;
		AudioSubObjectLayers& layers;
    };

	class AudioLayersEditorLayer : public UIWidget {
    public:
        AudioLayersEditorLayer(UIFactory& factory, AudioLayersEditor& layersEditor, size_t idx);

		void onMakeUI() override;

	private:
		UIFactory& factory;
        AudioLayersEditor& layersEditor;
		size_t idx;
    };
}
