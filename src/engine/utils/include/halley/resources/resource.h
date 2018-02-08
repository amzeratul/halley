#pragma once

#include <atomic>
#include <thread>
#include <array>
#include <condition_variable>
#include "metadata.h"
#include "halley/text/string_converter.h"

namespace Halley
{
	enum class ImportAssetType
	{
		Undefined,
		Skip,
		Codegen,
		SimpleCopy,
		Font,
		BitmapFont,
		Image,
		Texture,
		Material,
		Animation,
		Config,
		Audio,
		Sprite,
		SpriteSheet,
		Shader
	};

	enum class AssetType
	{
		BinaryFile,
		TextFile,
		ConfigFile,
		Texture,
		Image,
		MaterialDefinition,
		Sprite,
		SpriteSheet,
		Animation,
		Font,
		AudioClip,
		Shader
	};

	template <>
	struct EnumNames<AssetType> {
		constexpr std::array<const char*, 12> operator()() const {
			return{{
				"binaryFile",
				"textFile",
				"configFile",
				"texture",
				"image",
				"materialDefinition",
				"sprite",
				"spriteSheet",
				"animation",
				"font",
				"audioClip",
				"shader"
			}};
		}
	};

	class Resource
	{
	public:
		virtual ~Resource() {}

		void setMeta(const Metadata& meta);
		const Metadata& getMeta() const;
		void setAssetId(const String& name);
		const String& getAssetId() const;

	private:
		Metadata meta;
		String assetId;
	};

	class AsyncResource : public Resource
	{
	public:
		AsyncResource();
		virtual ~AsyncResource();

		void startLoading(); // call from main thread before spinning worker thread
		void doneLoading();  // call from worker thread when done loading
		void loadingFailed(); // Call from worker thread if loading fails
		void waitForLoad() const;

		bool isLoaded() const;

	private:
		std::atomic<bool> failed;
		std::atomic<bool> loading;
		mutable std::condition_variable loadWait;
		mutable std::mutex loadMutex;
	};
}
