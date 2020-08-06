#pragma once

#include "resources.h"

namespace Halley {
	template <typename T>
	class ResourceReference {
	public:
		ResourceReference() = default;
		
		ResourceReference(std::shared_ptr<const T> resource)
			: resource(resource)
		{}

		bool hasValue() const { return !!resource; }
		operator bool() const { return !!resource; }

		std::shared_ptr<const T>& get() { return resource; }
		const std::shared_ptr<const T>& get() const { return resource; }

		const String& getId() const
		{
			if (!resource) {
				const static String dummyId;
				return dummyId;
			}
			return resource->getAssetId();
		}

	private:
		std::shared_ptr<const T> resource;
	};
}
