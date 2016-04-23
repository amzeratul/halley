#pragma once

namespace YAML
{
	class Node;
}

namespace Halley
{
	class SystemSchema
	{
	public:
		SystemSchema();
		explicit SystemSchema(YAML::Node node);
	};
}