#pragma once
#include "entity.h"
#include "halley/bytes/config_node_serializer_base.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class DataInterpolatorSet {
	public:
		void setInterpolator(std::unique_ptr<IDataInterpolator> interpolator, EntityId entity, std::string_view componentName, std::string_view fieldName);
		IDataInterpolator* tryGetInterpolator(EntityId entity, std::string_view componentName, std::string_view fieldName);

		void update(Time time) const;

	private:
		using Key = std::tuple<EntityId, std::string_view, std::string_view>;

		Vector<std::pair<Key, std::unique_ptr<IDataInterpolator>>> interpolators; // Vector as hashing this is complex and we only expect a few interpolators per entity

		Key makeKey(EntityId entity, std::string_view componentName, std::string_view fieldName) const;
	};

	class DataInterpolatorSetRetriever : public IDataInterpolatorSet {
	public:
		DataInterpolatorSetRetriever(EntityRef rootEntity);
		
		IDataInterpolator* tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const override;

	private:
		DataInterpolatorSet* dataInterpolatorSet = nullptr;
	};

	template <typename T>
	class DataInterpolator : public IDataInterpolator {
	public:
		void deserialize(void* value, const void* defaultValue, const EntitySerializationContext& context, const ConfigNode& node) override
		{
			if (enabled) {
				doDeserialize(*static_cast<T*>(value), *static_cast<const T*>(defaultValue), context, node);
			}
		}
		
		void setEnabled(bool enabled) override { this->enabled = enabled; }
		bool isEnabled() const override { return enabled; }

	protected:
		void doDeserialize(T& value, const T& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
		{
			ConfigNodeSerializer<T>::deserialize(value, defaultValue, context, node);
		}

	private:
		bool enabled = true;
	};
}
