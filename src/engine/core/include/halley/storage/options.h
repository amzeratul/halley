#pragma once
#include "halley/api/save_data.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class AudioAPI;

	class Options {
	public:
		Options(std::shared_ptr<ISaveData> saveData);
		virtual ~Options() = default;

		void load();
		void save();
		void reset();

		float getVolume(std::string_view bus) const;
		void setVolume(std::string_view bus, float volume);
		void applyVolumes(AudioAPI& audio);
		String getKeyboardLayout() const;
		void setKeyboardLayout(String layout);

		void setDevValue(std::string_view name, float value);
		float getDevValue(std::string_view name, float defaultValue) const;

		void setDevFlag(std::string_view name, bool value);
		bool getDevFlag(std::string_view name, bool defaultValue) const;

	protected:
		
		std::shared_ptr<ISaveData> saveData;

		std::map<String, float> volume;
		std::map<String, bool> devFlags;
		std::map<String, float> devValues;
		String keyboardLayout;
		
		virtual void onReset();
		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;
	};
}
