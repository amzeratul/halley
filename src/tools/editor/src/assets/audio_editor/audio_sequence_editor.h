#pragma once

#include "halley/audio/sub_objects/audio_sub_object_sequence.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;

	class AudioSequenceEditor : public UIWidget {
    public:
        AudioSequenceEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSequence& sequence);

		void onMakeUI() override;

		void markModified(size_t idx);
        AudioObjectEditor& getEditor();
        AudioSubObjectSequence::Segment& getSegment(size_t idx);

    private:
		UIFactory& factory;
        AudioObjectEditor& editor;
		AudioSubObjectSequence& sequence;

		void refreshIds();
    };

	class AudioSequenceEditorSegment : public UIWidget {
    public:
        AudioSequenceEditorSegment(UIFactory& factory, AudioSequenceEditor& sequenceEditor, size_t idx);

		void onMakeUI() override;
		void setIdx(size_t idx);

	private:
		UIFactory& factory;
        AudioSequenceEditor& sequenceEditor;
		size_t idx;
    };
}
