#pragma once

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentMemberSchema
	{
	public:
		String name;
		String type;
		bool isConst;
	};

	class ComponentSchema
	{
	public:
		ComponentSchema();
		explicit ComponentSchema(YAML::Node node);

		int id = -1;
		String name;
		std::vector<ComponentMemberSchema> members;
	};
}