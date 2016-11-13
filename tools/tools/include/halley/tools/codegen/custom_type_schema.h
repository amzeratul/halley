#pragma once

#include <halley/text/halleystring.h>

namespace YAML
{
	class Node;
}

namespace Halley
{
	class CustomTypeSchema
	{
	public:
		CustomTypeSchema();
		explicit CustomTypeSchema(YAML::Node node);

		String name;
        String namespaceName;
		String includeFile;
	};
}
