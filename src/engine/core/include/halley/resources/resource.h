#pragma once

#include <atomic>
#include <thread>
#include <array>
#include <condition_variable>
#include "metadata.h"
#include "halley/concurrency/future.h"
#include "halley/text/enum_names.h"
#include "halley/time/halleytime.h"

#if defined(DEV_BUILD) && !defined(NN_NINTENDO_SDK)
#define ENABLE_HOT_RELOAD
#endif

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
		MaterialDefinition,
		Animation,
		ConfigFile,
		AudioClip,
		AudioObject,
		AudioEvent,
		Sprite,
		SpriteSheet,
		Shader,
		Mesh,
		VariableTable,
		GameProperties,
		RenderGraphDefinition,
		ScriptGraph,
		NavmeshSet,
		Prefab,
		Scene,
		UIDefinition
	};

	template <>
	struct EnumNames<ImportAssetType> {
		constexpr auto operator()() const {
			return std::to_array({
				"undefined",
				"skip",
				"codegen",
				"simpleCopy",
				"font",
				"bitmapFont",
				"image",
				"texture",
				"materialDefinition",
				"animation",
				"configFile",
				"audioClip",
				"audioObject",
				"audioEvent",
				"sprite",
				"spriteSheet",
				"shader",
				"mesh",
				"variableTable",
				"gameProperties",
				"renderGraphDefinition",
				"scriptGraph",
				"navmeshSet",
				"prefab",
				"scene",
				"uiDefinition"
			});
		}
	};

	// This order matters.
	// Assets which depend on other types should show up on the list AFTER
	// e.g. since materials depend on shaders, they show after shaders
	enum class AssetType
	{
		BinaryFile,
		TextFile,
		ConfigFile,
		GameProperties,
		Texture,
		Shader,
		MaterialDefinition, // Depends on Texture and Shader
		Image,
		SpriteSheet, // Depends on MaterialDefinition
		Sprite, // Depends on SpriteSheet
		Animation, // Depends on SpriteSheet
		Font, // Depends on SpriteSheet
		AudioClip,
		AudioObject, // Depends on AudioClip
		AudioEvent, // Depends on AudioObject
		Mesh,
		MeshAnimation, // Depends on Mesh
		VariableTable,
		RenderGraphDefinition,
		ScriptGraph,
		NavmeshSet,
		Prefab,
		Scene,
		UIDefinition
	};

	template <>
	struct EnumNames<AssetType> {
		constexpr auto operator()() const {
			return std::to_array({
				"binaryFile",
				"textFile",
				"configFile",
				"gameProperties",
				"texture",
				"shader",
				"materialDefinition",
				"image",
				"spriteSheet",
				"sprite",
				"animation",
				"font",
				"audioClip",
				"audioObject",
				"audioEvent",
				"mesh",
				"meshAnimation",
				"variableTable",
				"renderGraphDefinition",
				"scriptGraph",
				"navmeshSet",
				"prefab",
				"scene",
				"uiDefinition"
			});
		}
	};

	class ResourceObserver;
	class Resources;

	struct ResourceMemoryUsage {
		size_t ramUsage = 0;
		size_t vramUsage = 0;

		ResourceMemoryUsage& operator+=(const ResourceMemoryUsage& other)
		{
			ramUsage += other.ramUsage;
			vramUsage += other.vramUsage;
			return *this;
		}

		ResourceMemoryUsage operator+(const ResourceMemoryUsage& other) const
		{
			ResourceMemoryUsage result = *this;
			result.ramUsage += other.ramUsage;
			result.vramUsage += other.vramUsage;
			return result;
		}

		bool operator<(const ResourceMemoryUsage& other) const
		{
			const auto t0 = getTotal();
			const auto t1 = other.getTotal();

			if (t0 != t1) {
				return t0 < t1;
			} else if (vramUsage != other.vramUsage) {
				return vramUsage < other.vramUsage;
			} else {
				return ramUsage < other.ramUsage;
			}
		}

		String toString() const
		{
			if (vramUsage > 0) {
				return String::prettySize(ramUsage + vramUsage) + " (" + String::prettySize(ramUsage) + " RAM + " + String::prettySize(vramUsage) + " VRAM)";
			} else {
				return String::prettySize(ramUsage);
			}
		}

		size_t getTotal() const
		{
			return ramUsage + vramUsage;
		}
	};

	class Resource
	{
	public:
		virtual ~Resource();

		void setMeta(Metadata meta);
		const Metadata& getMeta() const { return meta; }
		bool isMetaSet() const { return metaSet; }
		
		virtual void setAssetId(String name);
		virtual void setAssetIdx(uint32_t idx);
		const String& getAssetId() const { return assetId; }
		uint32_t getAssetIdx() const { return assetIdx; }

		virtual void onLoaded(Resources& resources);
		
		int getAssetVersion() const { return assetVersion; }
		void increaseAssetVersion();
		void reloadResource(Resource&& resource);

		virtual ResourceMemoryUsage getMemoryUsage() const;          /// How much memory it's using right now
		virtual ResourceMemoryUsage getEstimatedMemoryUsage() const; /// How much memory it'd use if it was loaded

		void increaseAge(float time);
		void resetAge();
		float getAge() const;

		virtual void startFrame(Time dt) const;

		void setUnloaded();
		bool isUnloaded() const;
		virtual void onOtherResourcesUnloaded();

	protected:
		virtual void reload(Resource&& resource);

	private:
		Metadata meta;
		String assetId;
		uint32_t assetIdx = 0;
		int assetVersion = 0;
		float age = 0;
		bool metaSet = false;
		bool unloaded = false;
	};

	class ResourceObserver
	{
	public:
		ResourceObserver();
		ResourceObserver(const Resource& res);
		virtual ~ResourceObserver();

		void startObserving(const Resource& res);
		void stopObserving();
		
		const Resource* getResourceBeingObserved() const;
		bool needsUpdate() const;
		virtual void update();

	private:
		const Resource* res = nullptr;
		int assetVersion = 0;
	};

	enum class ResourceDesiredLoadState {
		Undefined,
	    Load,		// Used very recently, keep loaded no matter what
        Preload,    // Not in use, but likely to be used soon. Can be loaded if there's budget, or unloaded if we're running out
        PreloadLowPriority,  // Not in use, might be used soon. Same as above, but lower priority for loading
        Stale,      // Not in use, unlikely to be used soon. Unload if we go over budget
		Unload		// Not in use
    };

	template <>
	struct EnumNames<ResourceDesiredLoadState> {
		constexpr auto operator()() const {
			return std::to_array({
				"undefined",
				"load",
				"preload",
				"preloadLowPriority",
				"stale",
				"unload"
			});
		}
	};

	class AsyncResource : public Resource
	{
	public:
		enum class State : uint8_t {
			Unloaded,
			Loading,
			Loaded
		};

		struct UsagePattern {
			Time timeSinceInUse = 0;
			Time timeSinceInBackground = 0;
			Time timeSinceInLowPriorityBackground = 0;
			int framesSinceInUse = 0;
			int framesSinceInBackground = 0;
			int framesSinceInLowPriorityBackground = 0;
			bool loaded = false;
		};

		AsyncResource();
		virtual ~AsyncResource();

		AsyncResource(const AsyncResource& other);
		AsyncResource(AsyncResource&& other) noexcept;
		AsyncResource& operator=(const AsyncResource& other);
		AsyncResource& operator=(AsyncResource&& other) noexcept;

		void startLoading(); // call from main thread before spinning worker thread
		void doneLoading();  // call from worker thread when done loading
		void doneUnloading(); // call from wherever when done unloading
		void loadingFailed(); // Call from worker thread if loading fails
		bool requestLoading() const;
		bool requestUnloading() const;
		virtual bool canUnload() const;
		void waitForLoad(bool acceptFailed = false) const;
		Future<void> onLoad() const;

		void startFrame(Time dt) const override;
		void markActivelyInUse() const;
		void markBackgroundLoaded() const;
		void markLowPriorityBackgroundLoaded() const;
		const UsagePattern& getUsagePattern() const;

		bool isLoaded() const;
		bool hasSucceeded() const;
		bool hasFailed() const;

		void setDesiredLoadState(ResourceDesiredLoadState state) const;
		ResourceDesiredLoadState getDesiredLoadState() const;

	protected:
		virtual bool doRequestLoading();
		virtual bool doRequestUnloading();

	private:
		std::atomic<bool> failed;
		std::atomic<State> loadState;
		mutable ResourceDesiredLoadState desiredLoadState = ResourceDesiredLoadState::Undefined;

		mutable ConditionVariable loadWait;
		mutable Mutex loadMutex;
		mutable Vector<Promise<void>> pendingPromises;

		mutable UsagePattern usageData;
		mutable std::atomic<bool> inUseThisFrame;
		mutable std::atomic<bool> inBackgroundThisFrame;
		mutable std::atomic<bool> inLowPriorityBackgroundThisFrame;
	};

	struct ResourceOptions {
		bool retainPixelData = false;
		bool retainShaderData = false;
		bool allowHotReload = true;
		
		ResourceOptions(bool retainPixelData = false, bool retainShaderData = false, bool allowHotReload = true)
			: retainPixelData(retainPixelData)
			, retainShaderData(retainShaderData)
			, allowHotReload(allowHotReload)
		{}
	};
}
