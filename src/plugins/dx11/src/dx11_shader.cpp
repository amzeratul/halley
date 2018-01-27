#include "dx11_shader.h"
#include "dx11_video.h"
using namespace Halley;

// TODO: REMOVE THESE TWO
#include <D3Dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")

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
	HRESULT result = D3DCompile2(bytes.data(), bytes.size(), name.c_str(), nullptr, nullptr, entry.c_str(), target.c_str(), 0, 0, 0, nullptr, 0, &codeBlob, &errorBlob);
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

void DX11Shader::setMaterialLayout(DX11Video& video, const std::vector<MaterialAttribute>& attributes)
{
	if (layout) {
		return;
	}

	Expects(vertexBlob);

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		//{"a_vertPos", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"a_color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12 * 4, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	HRESULT result = video.getDevice().CreateInputLayout(ied, 1, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &layout);
	if (result != S_OK) {
		throw Exception("Unable to create input layout for shader " + name);
	}
	vertexBlob->Release();
	vertexBlob = nullptr;
}
