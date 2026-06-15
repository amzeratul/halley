#pragma once

#include "resources.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ResourceUnloaderAssetTypeRules {
    public:
        size_t budget = 1024 * 1024 * 1024;     /// Total byte budget for this asset type; can preload to that amount, and tries to keep all resources below it
        size_t staleBudget = 768 * 1024 * 1024; /// Unload stale objects when this usage is exceeded

        ResourceUnloaderAssetTypeRules() = default;
        ResourceUnloaderAssetTypeRules(size_t budget);
        ResourceUnloaderAssetTypeRules(size_t budget, size_t staleBudget);
    };

    class ResourceUnloaderRules {
    public:
        HashMap<AssetType, ResourceUnloaderAssetTypeRules> rules;
    };

    class ResourceUnloader {
		struct LoadStateInfo {
            std::shared_ptr<const AsyncResource> res;
            bool loaded = false;
            bool markAsLoading = false;
            bool markAsUnloading = false;
			ResourceDesiredLoadState desiredState = ResourceDesiredLoadState::Load;
            size_t memoryUsage = 0;
            Time timeSinceUse = 0;

            bool operator<(const LoadStateInfo& other) const;
		};

        struct StateCollection {
            size_t curMemoryUsage = 0;
	        Vector<LoadStateInfo> states;

            void sort();
        };

    public:
        ResourceUnloader(Resources& resources);

        void update(Time t, const ResourceUnloaderRules& rules);

    private:
        Resources& resources;
        Time budgetMessageTimeout = 0;
        Time unloadPreloadMessageTimeout = 0;
        uint32_t frameIdx = 1000;
        Vector<float> frameTimes;

        constexpr static float maxTimeLogged = 15.0; // seconds

        HashMap<ResourceDesiredLoadState, StateCollection> lastStates;

        Future<void> pendingUpdate;

        void prepareCollection(float t, ResourceCollectionBase& collection);
        void updateCollection(float t, ResourceCollectionBase& collection, const ResourceUnloaderAssetTypeRules& rules);
        void updateResourcesAndCollectStates(float t, ResourceCollectionBase& collection, HashMap<ResourceDesiredLoadState, StateCollection>& states);

        template<typename T>
        LoadStateInfo getStateInfo(const std::shared_ptr<Resource>& resource, float t) const;

        float getTimeSince(uint32_t idx) const;
        uint32_t getFramesSince(uint32_t idx) const;
    };
}
