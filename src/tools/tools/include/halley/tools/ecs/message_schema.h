#pragma once

#include "fields_schema.h"
#include <unordered_set>

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
		explicit MessageSchema(YAML::Node node, bool generate);

		int id = -1;
		String name;
		Vector<MemberSchema> members;
		std::unordered_set<String> includeFiles;
		bool generate = false;
	};
}
