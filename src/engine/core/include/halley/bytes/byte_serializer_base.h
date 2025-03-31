#pragma once

namespace Halley {
    class Serializer;
    class Deserializer;

    class ByteSerializationContext;

    class IByteDataInterpolator
    {
    public:
        virtual ~IByteDataInterpolator() = default;

        virtual void setEnabled(bool enabled) {};
        [[nodiscard]] virtual bool isEnabled() const { return true; }

        virtual void serialize(const void* value, size_t size, Serializer& serializer) {}
        virtual void deserialize(void* value, size_t size, Deserializer& deserializer) {}
    };

    class IByteDataInterpolatorSet
    {
    public:
        virtual ~IByteDataInterpolatorSet() = default;
        [[nodiscard]] virtual IByteDataInterpolator* tryGetInterpolator(const ByteSerializationContext& context, int componentIndex, std::string_view fieldName) const = 0;
    };

    class ByteSerializationContext
    {
    public:
		Resources* resources = nullptr;
        const IByteDataInterpolatorSet* interpolators = nullptr;
        // DEPRECATED: This is only used by helpers which use ConfigNode serialization as a fallback.
        EntitySerializationContext* entitySerializationContext = nullptr;
    };
}
