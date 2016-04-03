#pragma once

namespace Halley
{
	class ResourceLoader;

	class Shader : public Resource
	{
	public:
		void ensureLoaded();

		static std::unique_ptr<Shader> loadResource(ResourceLoader loader);
	};
}
