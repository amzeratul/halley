#include "halley/ui/ui_definition.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

UIDefinition::UIDefinition() = default;

UIDefinition::UIDefinition(ConfigNode node)
	: data(std::move(node))
{
}

UIDefinition::UIDefinition(ConfigFile file)
	: data(std::move(file))
{
}

std::unique_ptr<UIDefinition> UIDefinition::loadResource(ResourceLoader& loader)
{
	auto bytes = loader.getStatic(false);
	if (!bytes) {
		return {};
	}
	
	auto ui = std::make_unique<UIDefinition>();
	Deserializer s(bytes->getSpan());
	s >> *ui;

	return ui;
}

void UIDefinition::reload(Resource&& resource)
{
	// TODO
}

void UIDefinition::serialize(Serializer& s) const
{
	s << data;
}

void UIDefinition::deserialize(Deserializer& s)
{
	s >> data;
}

const ConfigNode& UIDefinition::getRoot() const
{
	return data.getRoot();
}

ConfigNode& UIDefinition::getRoot()
{
	return data.getRoot();
}
