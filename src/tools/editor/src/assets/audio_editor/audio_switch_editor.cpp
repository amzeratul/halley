#include "audio_switch_editor.h"

#include "audio_object_editor.h"
#include "halley/audio/sub_objects/audio_sub_object_switch.h"
#include "halley/core/properties/audio_properties.h"
#include "halley/ui/widgets/ui_dropdown.h"

using namespace Halley;

AudioSwitchEditor::AudioSwitchEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSwitch& switchConfig)
	: UIWidget("audio_switch_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, switchConfig(switchConfig)
{
	factory.loadUI(*this, "halley/audio_editor/audio_switch_editor");
}

void AudioSwitchEditor::onMakeUI()
{
	getWidgetAs<UIDropdown>("switchId")->setOptions(editor.getAudioProperties().getSwitchIds());

	bindData("switchId", switchConfig.getSwitchId(), [=] (String value)
	{
		switchConfig.setSwitchId(std::move(value));
		editor.markModified();
	});
}
