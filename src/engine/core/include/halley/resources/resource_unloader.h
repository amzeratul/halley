#pragma once

#include "resources.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ResourceUnloader {
    public:
        ResourceUnloader(Resources& resources);

        void setBudget(AssetType type, size_t maxSize);
        void update(Time t);

    private:
        Resources& resources;

        HashMap<AssetType, size_t> budgets;

        void updateCollection(Time t, ResourceCollectionBase& collection, size_t sizeLimit);
    };
}
