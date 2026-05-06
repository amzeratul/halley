#pragma once

#include "halley/entity/entity.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {

    class ByteDataInterpolatorSet : public IByteDataInterpolatorSet
    {
    public:
		void setInterpolator(std::shared_ptr<IByteDataInterpolator> interpolator, EntityId entityId, int componentIndex, std::string_view fieldName);
        [[nodiscard]] IByteDataInterpolator* tryGetInterpolator(EntityId entityId, int componentIndex, std::string_view fieldName) const override;

        void reset() const override;
        void update(Time t, World& world) const override;

    private:
		using Key = std::tuple<EntityId, int, std::string_view>;
        Vector<std::pair<Key, std::shared_ptr<IByteDataInterpolator>>> interpolators;
    };

    template <typename T>
    class ByteDataInterpolator : public IByteDataInterpolator
    {
    public:
        void setEnabled(bool enable) override
        {
            this->enabled = enable;
        }

        [[nodiscard]] bool isEnabled() const override
        {
            return enabled;
        }

        void serialize(const void* value, size_t size, Serializer& serializer) override
        {
            HalleyAssertDev(size == sizeof(T));

            serializer << enabled;

            if (enabled) {
                doSerialize(*static_cast<const T*>(value), serializer);
            }
        }

        void deserialize(void* value, size_t size, Deserializer& deserializer) override
        {
            HalleyAssertDev(size == sizeof(T));

            bool hasPayload; // TODO: should this set the 'enabled' flag?
            deserializer >> hasPayload;

            if (hasPayload) {
                doDeserialize(*static_cast<T*>(value), deserializer);
            }
        }

        virtual void doSerialize(const T& value, Serializer& serializer) = 0;
        virtual void doDeserialize(T& value, Deserializer& deserializer) = 0;

    protected:
        template <typename Intermediate = T, typename Discrete = T>
        static Discrete quantize(const Intermediate value, Intermediate granularity)
        {
            return static_cast<Discrete>(value * granularity);
        }

        template <typename Intermediate = T, typename Discrete = T>
        static Intermediate dequantize(const Discrete value, Intermediate granularity)
        {
            return static_cast<Intermediate>(value) / granularity;
        }

        template <typename Intermediate = T, typename Discrete = T>
        Discrete quantize(const Intermediate value, Intermediate minValue, Intermediate maxValue, Intermediate granularity)
        {
            const Intermediate n = std::clamp(value, minValue, maxValue);
            const Intermediate range = maxValue - minValue;
            const Discrete result = static_cast<Discrete>((n - minValue) / range * granularity);
            return result;
        }

        template <typename Intermediate = T, typename Discrete = T>
        Intermediate dequantize(const Discrete value, Intermediate minValue, Intermediate maxValue, Intermediate granularity)
        {
            const Intermediate range = maxValue - minValue;
            const Intermediate result = minValue + (static_cast<Intermediate>(value) / granularity * range);
            return result;
        }

    private:
        bool enabled = true;
    };

}
