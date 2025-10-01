#include "audio_playback_panel.h"

#include "halley/properties/audio_properties.h"
#include "halley/tools/project/project.h"

using namespace Halley;

AudioPlaybackPanel::AudioPlaybackPanel(UIFactory& factory, const HalleyAPI& api, Project& project)
	: UIWidget("audio_playback", {}, UISizer())
	, factory(factory)
	, api(api)
	, project(project)
{
	factory.loadUI(*this, "halley/audio_editor/audio_playback_panel");
}

void AudioPlaybackPanel::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "play", [=] (const UIEvent& event) {
		onPlay();
	});

	playButton = getWidgetAs<UIButton>("play");
}

void AudioPlaybackPanel::update(Time t, bool moved)
{
	if (needsIconUpdate) {
		const char* image = isPlaying() ? "halley_ui/icon_pause.png" : "halley_ui/icon_play.png";
		playButton->setIcon(Sprite().setImage(factory.getResources(), image));
	}
	playButton->setEnabled(isReadyToPlay());

	if (needsObjectUpdate && updateCooldown <= 0) {
		needsObjectUpdate = false;
		updateCooldown = 0.1;
		updatePlaybackObject();
	}
	if (updateCooldown > 0) {
		updateCooldown -= t;
	}

	if (needsLoadingVariables && isReadyToPlay()) {
		loadVariables();
	}

	if (emitter) {
		emitter->setPosition(audioPosition);
	}
}

void AudioPlaybackPanel::onActiveChanged(bool active)
{
	if (!active) {
		pause();
	}
}

void AudioPlaybackPanel::onObjectModified()
{
	if (object) {
		needsObjectUpdate = true;
	}

	loadVariables();
}

void AudioPlaybackPanel::setAudioObject(std::shared_ptr<const AudioObject> object)
{
	this->object = std::move(object);
	event = {};
	loadVariables();
	updatePlaybackObject();
}

void AudioPlaybackPanel::setAudioEvent(std::shared_ptr<const AudioEvent> event)
{
	this->event = std::move(event);
	object = {};
	needsObjectUpdate = false;
	loadVariables();
	updatePlaybackObject();
}

void AudioPlaybackPanel::onPlay()
{
	needsIconUpdate = true;
	if (isPlaying()) {
		pause();
	} else {
		play();
	}
}

void AudioPlaybackPanel::play()
{
	if (!isReadyToPlay()) {
		return;
	}

	if (isPlaying()) {
		pause();
	}

	if (!emitter) {
		emitter = api.audio->createEmitter(audioPosition);
		applyVariablesToEmitter();

		api.audio->setListener(AudioListenerData(Vector3f()));
	}

	api.audio->resetBuses();

	if (object) {
		audioHandle = api.audio->play(playbackObject, emitter);
	} else if (event) {
		audioHandle = api.audio->postEvent(*event, emitter);
	}
}

void AudioPlaybackPanel::pause()
{
	if (!isPlaying()) {
		return;
	}

	audioHandle->stop();
	audioHandle = {};
}

bool AudioPlaybackPanel::isPlaying() const
{
	return static_cast<bool>(audioHandle);
}

bool AudioPlaybackPanel::isReadyToPlay() const
{
	return project.areAssetsLoaded();
}

void AudioPlaybackPanel::updatePlaybackObject()
{
	if (object) {
		if (playbackObject) {
			// Update existing
			auto newObject = AudioObject(*object);
			newObject.loadDependencies(project.getGameResources());

			// Note that this is only safe because it waits for the callback to complete
			api.audio->runOnAudioThread([&] () {
				playbackObject->reload(std::move(newObject));
			}).wait();
		} else {
			// Create new
			playbackObject = std::make_shared<AudioObject>(*object);
			playbackObject->loadDependencies(project.getGameResources());
		}
	} else {
		playbackObject = {};
	}
}

void AudioPlaybackPanel::loadVariables()
{
	if (!isReadyToPlay()) {
		needsLoadingVariables = true;
		return;
	}
	needsLoadingVariables = false;

	auto vars = getCurrentVariableList();
	if (vars != variables) {
		variables = std::move(vars);
		populateVariables();
	}
}

void AudioPlaybackPanel::populateVariables()
{
	auto container = getWidget("variablesContainer");
	container->clear();

	auto labelStyle = factory.getStyle("label");

	for (const auto& [varType, varId]: variables) {
		container->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromUserString(varId + ":")), 1, Vector4f(0, 0, 4, 0), UISizerAlignFlags::Right | UISizerAlignFlags::CentreVertical);
		container->add(makeVariableControl(varType, varId), 1, {}, UISizerAlignFlags::CentreVertical | UISizerFillFlags::FillHorizontal);
	}

	getWidget("variablesArea")->setActive(!variables.empty());
}

std::shared_ptr<UIWidget> AudioPlaybackPanel::makeVariableControl(VariableType type, const String& id)
{
	if (type == VariableType::Variable) {
		const auto style = factory.getStyle("slider");

		const auto& properties = api.audio->getAudioProperties();
		if (const auto* variable = properties.tryGetVariable(id)) {
			const auto range = variable->getRange();
			const auto startValue = curVars.value_or(id, variable->getDefaultValue());

			auto control = std::make_shared<UISlider>(id, style, range.start, range.end, startValue, true, true);
			control->bindData(id, startValue, [=] (float value) {
				setVariable(id, value);
			});
			return control;
		}
	} else if (type == VariableType::Switch) {
		const auto style = factory.getStyle("dropdownLight");

		const auto& properties = api.audio->getAudioProperties();
		if (const auto* swi = properties.tryGetSwitch(id)) {
			Vector<UIDropdown::Entry> options;
			for (const auto& v: swi->getValues()) {
				options += UIDropdown::Entry(v, LocalisedString::fromUserString(v));
			}
			const auto startValue = curSwitches.value_or(id, swi->getDefaultValue());

			auto control = std::make_shared<UIDropdown>(id, style);
			control->setOptions(std::move(options));
			control->bindData(id, startValue, [=] (String value) {
				setSwitch(id, value);
			});
			return control;
		}
	}
	return {};
}

void AudioPlaybackPanel::applyVariablesToEmitter()
{
	if (emitter) {
		for (const auto& [id, value]: curVars) {
			emitter->setVariable(id, value);
		}
		for (const auto& [id, value]: curSwitches) {
			emitter->setSwitch(id, value);
		}
	}
}

void AudioPlaybackPanel::setVariable(const String& id, float value)
{
	if (emitter) {
		emitter->setVariable(id, value);
	}
	curVars[id] = value;
}

void AudioPlaybackPanel::setSwitch(const String& id, const String& value)
{
	if (emitter) {
		emitter->setSwitch(id, value);
	}
	curSwitches[id] = value;
}

Vector<std::pair<AudioPlaybackPanel::VariableType, String>> AudioPlaybackPanel::getCurrentVariableList() const
{
	Vector<String> variables;
	Vector<String> switches;

	if (object) {
		object->collectVariablesUsed(variables, switches);
	}

	Vector<std::pair<VariableType, String>> result;
	for (auto& var: variables) {
		result += { VariableType::Variable, std::move(var) };
	}
	for (auto& sw: switches) {
		result += { VariableType::Switch, std::move(sw) };
	}

	return result;
}
