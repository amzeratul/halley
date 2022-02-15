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

	if (destination == SystemMessageDestination::AllClients || destination == SystemMessageDestination::RemoteClients) {
		if (!multicast) {
			throw Exception("Non-multicast system message \"" + name + "\" cannot be sent to all clients or remote clients.", HalleyExceptions::Tools);
		}
	}
	if (destination != SystemMessageDestination::Local) {
		if (!serializable) {
			throw Exception("System message \"" + name + "\" must be serializable since it's sent over the network.", HalleyExceptions::Tools);
		}
	}
}
