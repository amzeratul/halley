#pragma once

namespace Halley {
    class ComponentReflector {
    public:
    	virtual ~ComponentReflector() = default;

    	virtual const char* getName() const = 0;
    	virtual ConfigNode serialize(const EntitySerializationContext& context, const Component& component) const = 0;
    };

	template <typename T>
	class ComponentReflectorImpl : public ComponentReflector {
	public:
		const char* getName() const override
		{
			return T::componentName;
		}
		
		ConfigNode serialize(const EntitySerializationContext& context, const Component& component) const override
		{
			return static_cast<const T&>(component).serialize(context);
		}
	};
}
