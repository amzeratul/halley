#include "data_interpolator.h"

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include <components/network_component.h>

#include "entity_factory.h"

using namespace Halley;

void DataInterpolatorSet::setInterpolator(std::unique_ptr<IDataInterpolator> interpolator, EntityId entity, std::string_view componentName, std::string_view fieldName)
{
	const auto key = makeKey(entity, componentName, fieldName);
	for (auto& entry: interpolators) {
		if (entry.first == key) {
			entry.second = std::move(interpolator);
			return;
		}
	}
	
	interpolators.emplace_back(key, std::move(interpolator));
}

IDataInterpolator* DataInterpolatorSet::tryGetInterpolator(EntityId entity, std::string_view componentName, std::string_view fieldName)
{
	if (interpolators.empty()) {
		return nullptr;
	}

	const auto key = makeKey(entity, componentName, fieldName);
	for (auto& entry: interpolators) {
		if (entry.first == key) {
			return entry.second.get();
		}
	}
	return nullptr;
}

void DataInterpolatorSet::update(Time time) const
{
	for (auto& e: interpolators) {
		e.second->update(time);
	}
}

DataInterpolatorSet::Key DataInterpolatorSet::makeKey(EntityId entity, std::string_view componentName, std::string_view fieldName) const
{
	return Key(entity, componentName, fieldName);
}

DataInterpolatorSetRetriever::DataInterpolatorSetRetriever(EntityRef rootEntity)
{
	auto* networkComponent = rootEntity.tryGetComponent<NetworkComponent>();
	if (networkComponent) {
		dataInterpolatorSet = &networkComponent->dataInterpolatorSet;
	}
}

IDataInterpolator* DataInterpolatorSetRetriever::tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const
{
	if (dataInterpolatorSet) {
		return dataInterpolatorSet->tryGetInterpolator(context.entityContext->getCurrentEntity().getEntityId(), componentName, fieldName);
	} else {
		return nullptr;
	}
}
