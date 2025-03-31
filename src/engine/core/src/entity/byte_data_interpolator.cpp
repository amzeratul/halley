#include "halley/entity/byte_data_interpolator.h"

using namespace Halley;

void ByteDataInterpolatorSet::setInterpolator(std::shared_ptr<IByteDataInterpolator> interpolator, int componentIndex, std::string_view fieldName)
{
    if (componentIndex >= interpolators.size()) {
        interpolators.resize(componentIndex + 1);
    }

    auto& table = interpolators[componentIndex];

    table.emplace_back(fieldName, interpolator);
}

IByteDataInterpolator* ByteDataInterpolatorSet::tryGetInterpolator(const ByteSerializationContext& context, int componentIndex, std::string_view fieldName) const
{
    if (componentIndex < interpolators.size()) {
        const auto& table = interpolators[componentIndex];

        for (const auto& pair : table) {
            if (pair.first == fieldName) {
                return pair.second.get();
            }
        }
    }

    return nullptr;
}
