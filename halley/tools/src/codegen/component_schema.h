#pragma once

#include "fields_schema.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentSchema
	{
	public:
		ComponentSchema();
		explicit ComponentSchema(YAML::Node node);

		int id = -1;
		String name;
		std::vector<VariableSchema> members;
	};
}