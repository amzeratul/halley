#pragma once
#include "../../../opengl/src/gl_utils.h"

namespace Halley
{
	class ResourceLoader;
	class Shader;
	class VideoAPI;

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		Material(std::shared_ptr<Shader> shader, VideoAPI* api);

		void bind();
		Shader& getShader() const;

		MaterialParameter& operator[](String name);
		Blend::Type getBlend() const;
		static std::unique_ptr<Material> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		std::shared_ptr<Shader> shader;
		std::vector<MaterialParameter> uniforms;
		bool dirty = false;

		void ensureLoaded();
	};
}
