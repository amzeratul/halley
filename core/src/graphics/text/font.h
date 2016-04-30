#pragma once

namespace Halley
{
	class Font : public Resource
	{
	public:
		static std::unique_ptr<Font> loadResource(ResourceLoader& loader);
	};
}
