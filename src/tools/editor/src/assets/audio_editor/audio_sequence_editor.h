#pragma once

#include "halley/audio/sub_objects/audio_sub_object_sequence.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;

	class AudioSequenceEditor : public UIWidget {
    public:
        AudioSequenceEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSequence& layers);

		void onMakeUI() override;

		void markModified(size_t idx);
        AudioObjectEditor& getEditor();

    private:
		UIFactory& factory;
        AudioObjectEditor& editor;
		AudioSubObjectSequence& sequence;
    };
}
