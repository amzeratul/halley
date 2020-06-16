#pragma once

#include "message_schema.h"

namespace Halley
{
	class SystemMessageSchema : public MessageSchema
	{
	public:
		SystemMessageSchema();
		explicit SystemMessageSchema(YAML::Node node, bool generate);

		String returnType;
		bool multicast = false;
	};
}
