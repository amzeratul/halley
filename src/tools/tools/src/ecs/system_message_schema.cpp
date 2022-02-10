#include "halley/tools/ecs/system_message_schema.h"

#include "halley/file_formats/halley-yamlcpp.h"
#include <halley/tools/ecs/message_schema.h>

#include "halley/text/string_converter.h"

using namespace Halley;

SystemMessageSchema::SystemMessageSchema() {}

SystemMessageSchema::SystemMessageSchema(YAML::Node node, bool generate)
	: MessageSchema(node, generate)
{
	returnType = node["returnType"].as<std::string>("void");
	multicast = node["multicast"].as<bool>(false);
	destination = fromString<SystemMessageDestination>(node["destination"].as<std::string>("local"));
}
