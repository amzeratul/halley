#pragma once
#include "entity.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class DataInterpolatorSet {
	public:
		void setInterpolator(std::unique_ptr<IDataInterpolator> interpolator, EntityId entity, std::string_view componentName, std::string_view fieldName);
		IDataInterpolator* tryGetInterpolator(EntityId entity, std::string_view componentName, std::string_view fieldName);

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
}
