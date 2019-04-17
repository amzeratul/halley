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
    switch (shaderDef.first) {
      case ShaderType::Pixel:
        fragment_func = [lib newFunctionWithName:@"pixel_func"];
        if (fragment_func == nil) {
          throw Exception("Shader for " + definition.name + " is missing a pixel function.", HalleyExceptions::VideoPlugin);
        }
        break;
      case ShaderType::Vertex:
        vertex_func = [lib newFunctionWithName:@"vertex_func"];
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
}

int MetalShader::getUniformLocation(const String& name, ShaderType type)
{
  return -1;
}

int MetalShader::getBlockLocation(const String& name, ShaderType type)
{
  return -1;
}

id<MTLFunction> MetalShader::getVertexFunc() {
  return vertex_func;
}

id<MTLFunction> MetalShader::getFragmentFunc() {
  return fragment_func;
}


