#include "halley/ui/ui_definition.h"

#include "halley/file_formats/yaml_convert.h"
#include "halley/maths/uuid.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

UIDefinition::UIDefinition() = default;

UIDefinition::UIDefinition(ConfigNode node)
	: data(std::move(node))
{
	assignIds(data.getRoot());
}

UIDefinition::UIDefinition(ConfigFile file)
	: data(std::move(file))
{
	assignIds(data.getRoot());
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
	auto& other = dynamic_cast<UIDefinition&>(resource);
	data = std::move(other.data);
}

void UIDefinition::serialize(Serializer& s) const
{
	s << data;
}

void UIDefinition::deserialize(Deserializer& s)
{
	s >> data;
	assignIds(getRoot());
}

const ConfigNode& UIDefinition::getRoot() const
{
	return data.getRoot();
}

ConfigNode& UIDefinition::getRoot()
{
	return data.getRoot();
}

UIDefinition::FindResult UIDefinition::findUUID(const String& id)
{
	return findUUID(nullptr, data.getRoot(), id);
}

UIDefinition::FindResult UIDefinition::findUUID(ConfigNode* parent, ConfigNode& node, const String& id)
{
	if (node["uuid"].asString() == id) {
		return FindResult{ &node, parent };
	}

	if (node.hasKey("children")) {
		for (auto& c: node["children"].asSequence()) {
			if (auto r = findUUID(&node, c, id); r.result != nullptr) {
				return r;
			}
		}
	}

	return FindResult{ nullptr, nullptr };
}

void UIDefinition::assignIds(ConfigNode& node)
{
	if (!node.hasKey("uuid") || node["uuid"].getType() == ConfigNodeType::Undefined) {
		node["uuid"] = UUID::generate().toString();
	}
	if (node.hasKey("children")) {
		for (auto& c: node["children"].asSequence()) {
			assignIds(c);
		}
	}
}

void UIDefinition::parseYAML(gsl::span<const gsl::byte> yaml)
{
	YAMLConvert::parseConfig(data, yaml);
}

String UIDefinition::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "uuid", "proportion", "border", "sizer", "widget", "children" }};
	return YAMLConvert::generateYAML(data.getRoot(), options);
}
