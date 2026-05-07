#pragma once

#include "message_schema.h"
#include "halley/entity/system_message.h"

namespace Halley
{
	class SystemMessageSchema : public MessageSchema
	{
	public:
		SystemMessageSchema();
		explicit SystemMessageSchema(YAML::Node node, bool generate);

		String returnType;
		SystemMessageDestinationType destination = SystemMessageDestinationType::Local;
		bool multicast = false;
	};
}
