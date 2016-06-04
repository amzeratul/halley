#pragma once

#include "fields_schema.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class MessageSchema
	{
	public:
		MessageSchema();
		explicit MessageSchema(YAML::Node node);

		int id = -1;
		String name;
		Vector<VariableSchema> members;
	};
}
