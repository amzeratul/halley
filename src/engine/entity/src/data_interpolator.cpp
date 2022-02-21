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

bool DataInterpolatorSet::setInterpolatorEnabled(EntityId entityId, std::string_view componentName, std::string_view fieldName, bool enabled)
{
	auto* interpolator = tryGetInterpolator(entityId, componentName, fieldName);
	if (interpolator) {
		interpolator->setEnabled(enabled);
		return true;
	}
	return false;
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

DataInterpolatorSetRetriever::DataInterpolatorSetRetriever(EntityRef rootEntity, bool shouldCollectUUIDs)
{
	auto* networkComponent = rootEntity.tryGetComponent<NetworkComponent>();
	if (networkComponent) {
		dataInterpolatorSet = &networkComponent->dataInterpolatorSet;

		if (shouldCollectUUIDs) {
			collectUUIDs(rootEntity);
		}
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

IDataInterpolator* DataInterpolatorSetRetriever::tryGetInterpolator(EntityId entityId, std::string_view componentName, std::string_view fieldName) const
{
	if (dataInterpolatorSet) {
		return dataInterpolatorSet->tryGetInterpolator(entityId, componentName, fieldName);
	} else {
		return nullptr;
	}
}

ConfigNode DataInterpolatorSetRetriever::createComponentDelta(const UUID& instanceUUID, const String& componentName, const ConfigNode& from, const ConfigNode& origTo) const
{
	const auto iter = uuids.find(instanceUUID);
	const EntityId entityId = iter != uuids.end() ? iter->second : EntityId();

	ConfigNode to = ConfigNode(origTo);
	for (const auto& [fieldName, fromValue]: from.asMap()) {
		auto* interpolator = tryGetInterpolator(entityId, componentName, fieldName);
		if (interpolator) {
			auto newValue = interpolator->prepareFieldForSerialization(fromValue, origTo[fieldName]);
			if (newValue) {
				to[fieldName] = std::move(newValue.value());
			}
		}
	}
	
	return ConfigNode::createDelta(from, to);
}

void DataInterpolatorSetRetriever::collectUUIDs(EntityRef entity)
{
	uuids[entity.getInstanceUUID()] = entity.getEntityId();
	for (auto c: entity.getChildren()) {
		collectUUIDs(c);
	}
}
