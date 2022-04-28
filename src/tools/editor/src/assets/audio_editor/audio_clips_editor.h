#pragma once

#include "halley/audio/sub_objects/audio_sub_object_clips.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;

	class AudioClipsEditor : public UIWidget {
    public:
        AudioClipsEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectClips& clips);

        void onMakeUI() override;

    private:
        UIFactory& factory;
        AudioObjectEditor& editor;
        AudioSubObjectClips& clips;
    };
}
