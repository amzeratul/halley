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

CreateComponentFunctionResult WorldReflection::createComponent(const EntityFactoryContext& context, const String& componentName, EntityRef& entity, const ConfigNode& componentData) const
{
	const auto id = componentMap.at(componentName);
	return componentReflectors[id]->createComponent(context, entity, componentData);
}

std::unique_ptr<System> WorldReflection::createSystem(const String& name) const
{
	return std::unique_ptr<System>(systemReflectors[systemMap.at(name)].createSystem());
}

std::unique_ptr<Message> WorldReflection::createMessage(int id) const
{
	return messageReflectors.at(id)->createMessage();
}

std::unique_ptr<Message> WorldReflection::createMessage(const String& name) const
{
	return createMessage(messageMap.at(name));
}

std::unique_ptr<SystemMessage> WorldReflection::createSystemMessage(int id) const
{
	return systemMessageReflectors.at(id)->createSystemMessage();
}

std::unique_ptr<SystemMessage> WorldReflection::createSystemMessage(const String& name) const
{
	return createSystemMessage(systemMessageMap.at(name));
}

ComponentReflector& WorldReflection::getComponentReflector(int id) const
{
	return *componentReflectors.at(id);
}

ComponentReflector& WorldReflection::getComponentReflector(const String& name) const
{
	return *componentReflectors[componentMap.at(name)];
}
