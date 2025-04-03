#pragma once
#include "entity.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {

    class ByteDataInterpolatorSet : public IByteDataInterpolatorSet
    {
    public:
		void setInterpolator(std::shared_ptr<IByteDataInterpolator> interpolator, EntityId entityId, int componentIndex, std::string_view fieldName);
        [[nodiscard]] IByteDataInterpolator* tryGetInterpolator(EntityId entityId, int componentIndex, std::string_view fieldName) const override;

        void update(Time t, World& world) const override;

    private:
		using Key = std::tuple<EntityId, int, std::string_view>;
        Vector<std::pair<Key, std::shared_ptr<IByteDataInterpolator>>> interpolators;
    };

}
