#pragma once

namespace Halley
{
	class ResourceLoader;

	class Shader : public Resource
	{
	public:
		virtual ~Shader() {}

		static std::unique_ptr<Shader> loadResource(ResourceLoader loader);
	};
}
