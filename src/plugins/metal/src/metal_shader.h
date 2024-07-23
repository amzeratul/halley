#pragma once
#include <halley/graphics/shader.h>
#include <Metal/Metal.h>

namespace Halley {

	class MetalVideo;

	class MetalShader final : public Shader
	{
	public:
		explicit MetalShader(MetalVideo& video, const ShaderDefinition& definition);
		~MetalShader();
		int getUniformLocation(const String& name, ShaderType stage) override;
		int getBlockLocation(const String& name, ShaderType stage) override;
		MTLRenderPipelineDescriptor* setupMaterial(const Material& material, id<MTLTexture> renderTargetTexture);

	private:
		MetalVideo& video;
		MTLRenderPipelineDescriptor* descriptor = nullptr;
		MTLVertexDescriptor* vertex_descriptor;
		id<MTLFunction> vertex_func;
		id<MTLFunction> fragment_func;

		MTLVertexFormat getVertexFormatForShaderParameterType(ShaderParameterType type);
	};
}
