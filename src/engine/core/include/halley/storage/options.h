#pragma once
#include "halley/api/save_data.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/i18n_language.h"

namespace Halley {
	class ControlBindings;
	class ControlBindingConfigs;
	class AudioAPI;

	enum class AudioOutputType {
		Headphones,
		StereoSpeakers,
		SurroundSpeakers
	};

	template <>
	struct EnumNames<AudioOutputType> {
		constexpr auto operator()() const {
			return std::to_array({
				"headphones",
				"stereoSpeakers",
				"surroundSpeakers"
			});
		}
	};

	class Options {
	public:
		Options(const HalleyAPI& api);
		virtual ~Options() = default;

		void load();
		void save();
		void reset();

		void update(Time t);

		bool isModified() const;
		void markModified();

		void setOption(std::string_view name, ConfigNode value);
		ConfigNode getOption(std::string_view name) const;
		bool hasOption(std::string_view name) const;

		void setDevValue(std::string_view name, float value);
		float getDevValue(std::string_view name, float defaultValue) const;

		void setDevFlag(std::string_view name, bool value);
		bool getDevFlag(std::string_view name, bool defaultValue) const;

		float getVolume(std::string_view bus) const;
		void setVolume(std::string_view bus, float volume);
		void applyVolumes(AudioAPI& audio);
		void setAudioOutputType(AudioOutputType type);
		AudioOutputType getAudioOutputType() const;

		String getKeyboardLayout() const;
		void setKeyboardLayout(String layout);
		I18NLanguage getLanguage() const;
		void setLanguage(I18NLanguage language);
		bool isLanguageSet() const;

		Vector2i getResolution(bool fullscreen) const;
		void setResolution(bool fullscreen, Vector2i resolution);
		bool getFullscreen() const;
		void setFullscreen(bool fullscreen);

		void setUISize(Vector2f uiSize);
		Vector2f getUISize() const;
		Vector2f getMaxOverscan(Vector2i resolution) const;

		bool getVibration() const;
		void setVibration(bool enabled);
		bool getFlashingEffects() const;
		void setFlashingEffects(bool enabled);
		bool getScreenShake() const;
		void setScreenShake(bool enabled);

		void setAudioEventLogging(std::optional<LoggerLevel> level, const std::optional<String>& prefix);
		void loadAudioEventLogging(AudioAPI& audioAPI) const;

		void loadControlBindings(ControlBindingConfigs config, bool devMode = false);
		ControlBindings& getControlBindings() const;
		void saveControlBindings();

	protected:
		
		std::shared_ptr<ISaveData> localContainer;
		std::shared_ptr<ISaveData> roamingContainer;
		std::shared_ptr<ControlBindings> controlBindings;

		Time saveCooldown = 0;

		ConfigNode options;
		bool modified = false;
		bool waitingForRoaming = false;
		
		virtual void onReset();

		void loadLocal();
		void loadRoaming();
		void load(ConfigNode node, bool roaming);

		ConfigNode toConfigNode(bool roaming) const;

		virtual bool isRoamingKey(const String& key) const;
	};
}
