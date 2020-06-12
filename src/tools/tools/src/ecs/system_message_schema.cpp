#include "halley/tools/ecs/system_message_schema.h"

#include "../yaml/halley-yamlcpp.h"
#include <halley/tools/ecs/message_schema.h>

using namespace Halley;

SystemMessageSchema::SystemMessageSchema() {}

SystemMessageSchema::SystemMessageSchema(YAML::Node node, bool generate)
	: MessageSchema(node, generate)
{
	returnType = node["returnType"].as<std::string>();
	async = node["async"].as<bool>(false);
}
