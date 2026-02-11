#include "halley/entity/world_reflection.h"
#include "halley/entity/system.h"

using namespace Halley;

WorldReflection::WorldReflection(CodegenFunctions& codegenFunctions)
{
	componentReflectors = codegenFunctions.makeComponentReflectors();
	systemReflectors = codegenFunctions.makeSystemReflectors();
	messageReflectors = codegenFunctions.makeMessageReflectors();
	systemMessageReflectors = codegenFunctions.makeSystemMessageReflectors();

	auto makeMap = [&] (auto& dst, auto& src)
	{
		for (int i = 0; i < static_cast<int>(src.size()); ++i) {
			dst[src[i]->getName()] = i;
		}
	};

	makeMap(componentMap, componentReflectors);
	makeMap(systemMap, systemReflectors);
	makeMap(messageMap, messageReflectors);
	makeMap(systemMessageMap, systemMessageReflectors);
}

CreateComponentFunctionResult WorldReflection::createComponent(const EntityFactoryContext& context, std::string_view componentName, EntityRef& entity, const ConfigNode& componentData) const
{
	const auto iter = componentMap.find(componentName);
	if (iter != componentMap.end()) {
		return componentReflectors[iter->second]->createComponent(context, entity, componentData);
	}
	return {};
}

std::unique_ptr<System> WorldReflection::createSystem(std::string_view name) const
{
	const auto iter = systemMap.find(name);
	if (iter != systemMap.end()) {
		return std::unique_ptr<System>(systemReflectors[iter->second].createSystem());
	} else {
		Logger::logError("Unknown system: " + String(name));
		return {};
	}
}

std::unique_ptr<Message> WorldReflection::createMessage(int id) const
{
	return messageReflectors.at(id)->createMessage();
}

std::unique_ptr<Message> WorldReflection::createMessage(std::string_view name) const
{
	auto iter = messageMap.find(name);
	if (iter != messageMap.end()) {
		return createMessage(iter->second);
	} else {
		throw Exception("Unknown Message \"" + String(name) + "\"", HalleyExceptions::Entity);
	}
}

std::unique_ptr<SystemMessage> WorldReflection::createSystemMessage(int id) const
{
	return systemMessageReflectors.at(id)->createSystemMessage();
}

std::unique_ptr<SystemMessage> WorldReflection::createSystemMessage(std::string_view name) const
{
	const auto iter = systemMessageMap.find(name);
	if (iter != systemMessageMap.end()) {
		return createSystemMessage(iter->second);
	} else {
		throw Exception("Unknown SystemMessage \"" + String(name) + "\"", HalleyExceptions::Entity);
	}
}

ComponentReflector& WorldReflection::getComponentReflector(int id) const
{
	return *componentReflectors.at(id);
}

const ComponentReflector* WorldReflection::tryGetComponentReflector(int id) const
{
	return componentReflectors[id].get();
}

ComponentReflector& WorldReflection::getComponentReflector(std::string_view name) const
{
	const auto iter = componentMap.find(name);
	if (iter != componentMap.end()) {
		return *componentReflectors[iter->second];
	} else {
		throw Exception("Unknown Component \"" + String(name) + "\"", HalleyExceptions::Entity);
	}
}

MessageReflector& WorldReflection::getMessageReflector(int id) const
{
	return *messageReflectors.at(id);
}

SystemMessageReflector& WorldReflection::getSystemMessageReflector(int id) const
{
	return *systemMessageReflectors.at(id);
}
