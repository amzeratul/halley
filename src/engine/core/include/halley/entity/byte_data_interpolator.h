#pragma once
#include "halley/bytes/byte_serializer.h"

namespace Halley {

    class ByteDataInterpolatorSet : public IByteDataInterpolatorSet
    {
    public:
		void setInterpolator(std::shared_ptr<IByteDataInterpolator> interpolator, int componentIndex, std::string_view fieldName);
        [[nodiscard]] IByteDataInterpolator* tryGetInterpolator(const ByteSerializationContext& context, int componentIndex, std::string_view fieldName) const override;

    private:
        using Field = std::pair<std::string_view, std::shared_ptr<IByteDataInterpolator>>;
        Vector<Vector<Field>> interpolators;
    };

}
