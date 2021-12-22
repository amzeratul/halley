#pragma once

#include "fields_schema.h"
#include "halley/data_structures/hash_map.h"

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
		HashSet<String> includeFiles;
		bool generate = false;
	};
}
