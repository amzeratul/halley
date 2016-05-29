#pragma once
#include "halley/text/halleystring.h"
#include <memory>
#include "halley/resources/resource.h"

namespace Halley
{
	class ResourceLoader;

	class TextFile : public Resource
	{
	public:
		TextFile();
		explicit TextFile(String data);

		String data;

		static std::unique_ptr<TextFile> loadResource(ResourceLoader& loader);
	};
}
