#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;
	class AudioSubObjectSwitch;

	class AudioSwitchEditor : public UIWidget {
    public:
        AudioSwitchEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSwitch& switchConfig);

	private:
		UIFactory& factory;
        AudioObjectEditor& editor;
		AudioSubObjectSwitch& switchConfig;
    };
}
