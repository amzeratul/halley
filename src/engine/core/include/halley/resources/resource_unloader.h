#pragma once

#include "resources.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ResourceUnloaderAssetTypeRules {
    public:
        size_t budget = 1024 * 1024 * 1024;

        ResourceUnloaderAssetTypeRules() = default;
        ResourceUnloaderAssetTypeRules(size_t budget);
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
            size_t memoryUsage;
            Time timeSinceUse = 0;

            bool operator<(const LoadStateInfo& other) const;
		};

        struct StateCollection {
            size_t curMemoryUsage;
	        Vector<LoadStateInfo> states;

            void sort();
        };

    public:
        ResourceUnloader(Resources& resources);

        void update(Time t, const ResourceUnloaderRules& rules);

    private:
        Resources& resources;

        void updateCollection(Time t, ResourceCollectionBase& collection, const ResourceUnloaderAssetTypeRules& rules);
        void updateResourcesAndCollectStates(Time t, ResourceCollectionBase& collection, HashMap<ResourceDesiredLoadState, StateCollection>& states);
    };
}
