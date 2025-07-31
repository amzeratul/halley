#include "halley/net/interpolators/byte_data_interpolator.h"

#include "halley/entity/entity.h"
#include "halley/entity/world.h"
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

void ByteDataInterpolatorSet::setInterpolator(std::shared_ptr<IByteDataInterpolator> interpolator, EntityId entityId, int componentIndex, std::string_view fieldName)
{
    const Key key = {entityId, componentIndex, fieldName};

    for (auto& entry : interpolators) {
        if (entry.first == key) {
            entry.second = std::move(interpolator);
            return;
        }
    }

    interpolators.emplace_back(key, std::move(interpolator));
}

IByteDataInterpolator* ByteDataInterpolatorSet::tryGetInterpolator(EntityId entityId, int componentIndex, std::string_view fieldName) const
{
    if (!interpolators.empty()) {
        const Key key = {entityId, componentIndex, fieldName};
        for (const auto& entry : interpolators) {
            if (entry.first == key) {
                return entry.second.get();
            }
        }
    }

    return nullptr;
}

void ByteDataInterpolatorSet::reset() const
{
    for (const auto& entry : interpolators) {
        entry.second->reset();
    }
}

void ByteDataInterpolatorSet::update(Time t, World& world) const
{
    for (const auto& entry : interpolators) {
        const bool modified = entry.second->update(t, world, std::get<0>(entry.first));

        // This hack is needed to make sure that transform 2D gets marked as dirty properly
        if (modified && std::get<1>(entry.first) == Transform2DComponent::componentIndex) {
            auto entity = world.getEntity(std::get<0>(entry.first));
            if (const auto transform = entity.tryGetComponent<Transform2DComponent>()) {
                transform->markDirty();
            }
        }
    }
}
