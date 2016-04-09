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
		explicit Material(ResourceLoader& loader);
		Material(std::shared_ptr<Shader> shader, VideoAPI* api);

		void bind();
		Shader& getShader() const;

		MaterialParameter& operator[](String name);
		Blend::Type getBlend() const { return blend; }
		static std::unique_ptr<Material> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		std::shared_ptr<Shader> shader;
		std::vector<MaterialParameter> uniforms;
		bool dirty = false;
		Blend::Type blend;

		void ensureLoaded();
	};
}
