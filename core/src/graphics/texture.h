#pragma once

namespace Halley
{
	class ResourceLoader;

	class Texture : public Resource
	{
	public:
		static std::unique_ptr<Texture> loadResource(ResourceLoader loader);
		
	private:
		unsigned int privateImpl;
	};
}
