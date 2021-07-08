#include "metal_shader.h"
#include "metal_video.h"

using namespace Halley;

MetalShader::MetalShader(MetalVideo& video, const ShaderDefinition& definition)
	: video(video)
{
	auto compileOptions = [MTLCompileOptions new];
	for (auto& shaderDef : definition.shaders) {
		auto shaderSrc = std::string(shaderDef.second.begin(), shaderDef.second.end());
		NSError* compileError;
		id<MTLLibrary> lib = [video.getDevice() newLibraryWithSource:[NSString stringWithUTF8String:shaderSrc.c_str()]
				options:compileOptions error:&compileError
		];
		if (compileError) {
			std::cout << "Metal shader compilation failed for material " << definition.name << std::endl;
			throw Exception([[compileError localizedDescription] UTF8String], HalleyExceptions::VideoPlugin);
		}
		auto func_names = [lib functionNames];
		if (func_names.count < 1) {
			throw Exception("Shader for " + definition.name + " has no functions", HalleyExceptions::VideoPlugin);
		}
		switch (shaderDef.first) {
			case ShaderType::Pixel:
				fragment_func = [lib newFunctionWithName:func_names[0]];
				if (fragment_func == nil) {
					throw Exception("Shader for " + definition.name + " is missing a pixel function.", HalleyExceptions::VideoPlugin);
				}
				break;
			case ShaderType::Vertex:
				vertex_func = [lib newFunctionWithName:func_names[0]];
				if (vertex_func == nil) {
					throw Exception("Shader for " + definition.name + " is missing a vertex function.", HalleyExceptions::VideoPlugin);
				}
				break;
			default:
				throw Exception("Unsupported shader type in " + definition.name + ": " + toString(shaderDef.first), HalleyExceptions::VideoPlugin);
		}
		[lib release];
		[compileError release];
	}
	[compileOptions release];
}

MetalShader::~MetalShader() {
	[vertex_func release];
	[fragment_func release];

	if (descriptor) {
		[vertex_descriptor release];
		[descriptor release];
	}
}

int MetalShader::getUniformLocation(const String& name, ShaderType type)
{
	return -1;
}

int MetalShader::getBlockLocation(const String& name, ShaderType type)
{
	return -1;
}

MTLRenderPipelineDescriptor* MetalShader::setupMaterial(const Material& material, id<MTLTexture> renderTargetTexture) {
	if (descriptor) {
		return descriptor;
	}

	descriptor = [[MTLRenderPipelineDescriptor alloc] init];
	descriptor.vertexFunction = vertex_func;
	descriptor.fragmentFunction = fragment_func;
	descriptor.label = [NSString stringWithUTF8String:material.getDefinition().getName().c_str()];
	descriptor.colorAttachments[0].pixelFormat = renderTargetTexture.pixelFormat;

	vertex_descriptor = [[MTLVertexDescriptor alloc] init];
	for (size_t i = 0; i < material.getDefinition().getAttributes().size(); ++i) {
		const auto& a = material.getDefinition().getAttributes()[i];
		vertex_descriptor.attributes[i].bufferIndex = MaxMetalBufferIndex;
		vertex_descriptor.attributes[i].offset = a.offset;
		vertex_descriptor.attributes[i].format = getVertexFormatForShaderParameterType(a.type);
	}
	vertex_descriptor.layouts[MaxMetalBufferIndex].stride = material.getDefinition().getVertexStride();
	descriptor.vertexDescriptor = vertex_descriptor;

	return descriptor;
}

MTLVertexFormat MetalShader::getVertexFormatForShaderParameterType(ShaderParameterType type) {
	switch (type) {
		case ShaderParameterType::Float:
			return MTLVertexFormatFloat;
		case ShaderParameterType::Float2:
			return MTLVertexFormatFloat2;
		case ShaderParameterType::Float3:
			return MTLVertexFormatFloat3;
		case ShaderParameterType::Float4:
			return MTLVertexFormatFloat4;
		case ShaderParameterType::Int:
			return MTLVertexFormatInt;
		case ShaderParameterType::Int2:
			return MTLVertexFormatInt2;
		case ShaderParameterType::Int3:
			return MTLVertexFormatInt3;
		case ShaderParameterType::Int4:
			return MTLVertexFormatInt4;
		default:
			throw Exception("Unknown shader parameter type: " + toString(int(type)), HalleyExceptions::VideoPlugin);
	}
}
