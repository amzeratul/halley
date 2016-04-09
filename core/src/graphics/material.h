#pragma once
#include "../../../opengl/src/gl_utils.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class MaterialPass;
	class ResourceLoader;
	class Shader;
	class VideoAPI;

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		explicit Material(ResourceLoader& loader);

		void bind(size_t pass);

		size_t getNumPasses() const;
		MaterialPass& getPass(size_t n);

		MaterialParameter& operator[](String name);
		static std::unique_ptr<Material> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		std::vector<MaterialParameter> uniforms;
		bool dirty = false;
		String name;
		std::vector<MaterialPass> passes;

		void ensureLoaded();
		void loadPass(YAML::Node node, std::function<String(String)> retriever);
	};

	class MaterialPass
	{
		friend class Material;

	public:
		MaterialPass(std::shared_ptr<Shader> shader, Blend::Type blend);

		void bind();

		Blend::Type getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }

	private:
		std::shared_ptr<Shader> shader;
		Blend::Type blend;
	};
}
