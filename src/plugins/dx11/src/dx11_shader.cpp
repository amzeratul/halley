#include "dx11_shader.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material_definition.h"
using namespace Halley;

// TODO: REMOVE THESE TWO
//#include <D3Dcompiler.h>
//#pragma comment(lib, "D3DCompiler.lib")

DX11Shader::DX11Shader(DX11Video& video, const ShaderDefinition& definition)
	: name(definition.name)
{
	for (auto& shader: definition.shaders) {
		loadShader(video, shader.first, shader.second);
	};
}

DX11Shader::~DX11Shader()
{
	if (vertexShader) {
		vertexShader->Release();
		vertexShader = nullptr;
	}
	if (pixelShader) {
		pixelShader->Release();
		pixelShader = nullptr;
	}
	if (geometryShader) {
		geometryShader->Release();
		geometryShader = nullptr;
	}
	if (layout) {
		layout->Release();
		layout = nullptr;
	}
	if (vertexBlob) {
		vertexBlob->Release();
		vertexBlob = nullptr;
	}
}

void DX11Shader::loadShader(DX11Video& video, ShaderType type, const Bytes& bytes)
{
	throw Exception("Loading HLSL is temporarily disabled until D3D shader situation is sorted");

	String entry;
	String target;

	switch (type) {
	case ShaderType::Vertex:
		entry = "VShader";
		target = "vs_4_0";
		break;

	case ShaderType::Pixel:
		entry = "PShader";
		target = "ps_4_0";
		break;

	case ShaderType::Geometry:
		entry = "GShader";
		target = "gs_4_0";
		break;

	default:
		throw Exception("Unsupported shader type: " + toString(type));
	}

	ID3D10Blob *codeBlob = nullptr;
	ID3D10Blob *errorBlob = nullptr;
	//HRESULT result = D3DCompile2(bytes.data(), bytes.size(), name.c_str(), nullptr, nullptr, entry.c_str(), target.c_str(), 0, 0, 0, nullptr, 0, &codeBlob, &errorBlob);
	HRESULT result = 100; // dunno
	if (result != S_OK) {
		String errorMessage = String(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
		errorBlob->Release();
		throw Exception("Failed to compile shader " + name + " (" + toString(type) + "):\n" + errorMessage);
	}

	switch (type) {
	case ShaderType::Vertex:
		result = video.getDevice().CreateVertexShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &vertexShader);
		vertexBlob = codeBlob;
		codeBlob = nullptr;
		break;

	case ShaderType::Pixel:
		result = video.getDevice().CreatePixelShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &pixelShader);
		break;

	case ShaderType::Geometry:
		result = video.getDevice().CreateGeometryShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &geometryShader);
		break;

	default:
		break;
	}

	if (codeBlob) {
		codeBlob->Release();
	}
	if (result != S_OK) {
		throw Exception("Unable to create shader " + name + " (" + toString(type) + ").");
	}
}

int DX11Shader::getUniformLocation(const String& name, ShaderType stage)
{
	return -1;
}

int DX11Shader::getBlockLocation(const String& name, ShaderType stage)
{
	return -1;
}

void DX11Shader::bind(DX11Video& video)
{
	Expects(vertexShader);
	Expects(layout);

	auto& devCon = video.getDeviceContext();
	devCon.VSSetShader(vertexShader, nullptr, 0);
	devCon.GSSetShader(geometryShader, nullptr, 0);
	devCon.PSSetShader(pixelShader, nullptr, 0);

	devCon.IASetInputLayout(layout);
}

static DXGI_FORMAT getDX11Format(ShaderParameterType type)
{
	switch (type) {
	case ShaderParameterType::Float:
		return DXGI_FORMAT_R32_FLOAT;
	case ShaderParameterType::Float2:
		return DXGI_FORMAT_R32G32_FLOAT;
	case ShaderParameterType::Float3:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case ShaderParameterType::Float4:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case ShaderParameterType::Int:
		return DXGI_FORMAT_R32_SINT;
	case ShaderParameterType::Int2:
		return DXGI_FORMAT_R32G32_SINT;
	case ShaderParameterType::Int3:
		return DXGI_FORMAT_R32G32B32_SINT;
	case ShaderParameterType::Int4:
		return DXGI_FORMAT_R32G32B32A32_SINT;
	default:
		throw Exception("Unknown shader parameter type: " + toString(int(type)));
	}
}

void DX11Shader::setMaterialLayout(DX11Video& video, const std::vector<MaterialAttribute>& attributes)
{
	if (layout) {
		return;
	}

	Expects(vertexBlob);

	std::vector<std::array<char, 64>> names(attributes.size());
	std::vector<D3D11_INPUT_ELEMENT_DESC> desc(attributes.size());

	for (size_t i = 0; i < desc.size(); ++i) {
		auto& a = attributes[i];
		
		UINT semanticIndex = 0;
		UINT inputSlot = 0;
		DXGI_FORMAT format = getDX11Format(a.type);
		UINT byteOffset = a.offset;

		String name = a.name.asciiUpper();
		if (name.startsWith("A_")) {
			name = name.mid(2);
		}
		if (name.right(1).isNumber()) {
			semanticIndex = name.right(1).toInteger();
			name = name.left(name.length() - 1);
		}
		strcpy_s(names[i].data(), 64, name.c_str());

		desc[i] = { names[i].data(), semanticIndex, format, inputSlot, byteOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	}

	HRESULT result = video.getDevice().CreateInputLayout(desc.data(), UINT(desc.size()), vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &layout);
	if (result != S_OK) {
		throw Exception("Unable to create input layout for shader " + name);
	}
	vertexBlob->Release();
	vertexBlob = nullptr;
}
