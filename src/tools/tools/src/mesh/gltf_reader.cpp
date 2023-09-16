#include "gltf_reader.h"

#include "halley/plugin/iasset_importer.h"
using namespace Halley;

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_IMAGE_PARSING // THIS IS A CUSTOM FLAG. KEEP THIS IN MIND WHEN UPDATING TINY_GLTF (If ever needed..)

#include "nlohmann/json.hpp"
#include "meshoptimizer/meshoptimizer.h"

__pragma(warning(push))
__pragma(warning(disable : 4018 4267))
#include "tinygltf/tiny_gltf.h"
__pragma(warning(pop))

String getMeshName(tinygltf::Model& model, int meshIndex, int primitiveIndex)
{
	String name = "MESH_" + toString(meshIndex) + "_" + model.meshes[meshIndex].name;
	const bool multiprim = model.meshes[meshIndex].primitives.size() > 1;
	if (multiprim) {
		name += "_PRIM_" + toString(primitiveIndex);
	}
	return name;
}

void unpackGLTFBuffer(tinygltf::Model& model, tinygltf::Accessor& accessor, Vector<uint8_t>& outputBuffer)
{
	const int bufferID = accessor.bufferView;
	const tinygltf::BufferView& bufferView = model.bufferViews[bufferID];

    size_t elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	tinygltf::Buffer& bufferData = (model.buffers[bufferView.buffer]);
	uint8_t* dataPtr = bufferData.data.data() + accessor.byteOffset + bufferView.byteOffset;
	const int components = tinygltf::GetNumComponentsInType(accessor.type);
	elementSize *= components;
	size_t stride = bufferView.byteStride;
	if (stride == 0) {
		stride = elementSize;
	}

	outputBuffer.resize(accessor.count * elementSize);

	for (int i = 0; i < accessor.count; i++) {
		const uint8_t* dataindex = dataPtr + stride * i;
		uint8_t* targetptr = outputBuffer.data() + elementSize * i;
		memcpy(targetptr, dataindex, elementSize);
	}
}

void extractGLTFVertices(tinygltf::Primitive& primitive, tinygltf::Model& model, Vector<VertexData>& vertices)
{
	tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes["POSITION"]];
	vertices.resize(positionAccessor.count);

	Vector<uint8_t> positions;
	unpackGLTFBuffer(model, positionAccessor, positions);
	for (int i = 0; i < vertices.size(); i++) {
		if (positionAccessor.type == TINYGLTF_TYPE_VEC3) {
			if (positionAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
				const float* positionsPtr = reinterpret_cast<float*>(positions.data());

				vertices[i].pos.x = *(positionsPtr + (i * 3) + 0);
				vertices[i].pos.y = *(positionsPtr + (i * 3) + 1);
				vertices[i].pos.z = *(positionsPtr + (i * 3) + 2);
				vertices[i].pos.w = 1.0f;
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}

	tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes["NORMAL"]];
	Vector<uint8_t> normalData;
	unpackGLTFBuffer(model, normalAccessor, normalData);

	for (int i = 0; i < vertices.size(); i++) {
		if (normalAccessor.type == TINYGLTF_TYPE_VEC3) {
			if (normalAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
				const float* normalsPtr = reinterpret_cast<float*>(normalData.data());

				vertices[i].normal.x = *(normalsPtr + (i * 3) + 0);
				vertices[i].normal.y = *(normalsPtr + (i * 3) + 1);
				vertices[i].normal.z = *(normalsPtr + (i * 3) + 2);
				vertices[i].normal.w = 1.0f;

				vertices[i].colour.x = *(normalsPtr + (i * 3) + 0);
				vertices[i].colour.y = *(normalsPtr + (i * 3) + 1);
				vertices[i].colour.z = *(normalsPtr + (i * 3) + 2);
				vertices[i].colour.w = 1.0f;
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}

	tinygltf::Accessor& texcoordsAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
	Vector<uint8_t> texcoordsData;
	unpackGLTFBuffer(model, texcoordsAccessor, texcoordsData);
	for (int i = 0; i < vertices.size(); i++) {
		if (texcoordsAccessor.type == TINYGLTF_TYPE_VEC2) {
			if (texcoordsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
				const float* texcoordsPtr = reinterpret_cast<float*>(texcoordsData.data());

				vertices[i].texCoord0.x = *(texcoordsPtr + (i * 2) + 0);
				vertices[i].texCoord0.y = *(texcoordsPtr + (i * 2) + 1);
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}
}

void extractGLTFIndices(tinygltf::Primitive& primitive, tinygltf::Model& model, Vector<IndexType>& primindices)
{
	const int indexaccessor = primitive.indices;
	const int componentType = model.accessors[indexaccessor].componentType;

    Vector<uint8_t> unpackedIndices;
	unpackGLTFBuffer(model, model.accessors[indexaccessor], unpackedIndices);

	for (int i = 0; i < model.accessors[indexaccessor].count; i++) {
		uint32_t index = 0;
		switch (componentType) {
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		{
			uint16_t* bfr = (uint16_t*)unpackedIndices.data();
			index = *(bfr + i);
		}
		break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		{
			int16_t* bfr = (int16_t*)unpackedIndices.data();
			index = *(bfr + i);
		}
		break;
		default:
			assert(false);
		}

		primindices.push_back(index);
	}

	for (int i = 0; i < primindices.size() / 3; i++) {
		//flip the triangle
		std::swap(primindices[i * 3 + 1], primindices[i * 3 + 2]);
	}
}

std::unique_ptr<Mesh> GLTFReader::parse(const ImportingAsset& asset, const Bytes& data)
{
	using namespace tinygltf;

	Model model;
	TinyGLTF loader;
	std::string err;
	std::string warn;
	
    bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, data.data(), static_cast<unsigned>(data.size()));
	if (!warn.empty()) {
		Logger::logWarning(warn.c_str());
	}
	if (!err.empty()) {
		Logger::logError(err.c_str());
	}
	if (!ret) {
		Logger::logError("Failed to parse glTF");
		return {};
	}

	auto mesh = std::make_unique<Mesh>();
	mesh->setMaterialName("Halley/StandardMesh"); // TODO
	mesh->setTextureNames({ "checker.png" }); // TODO

	for (auto meshindex = 0; meshindex < model.meshes.size(); meshindex++) {
		auto& glmesh = model.meshes[meshindex];

		for (auto primindex = 0; primindex < glmesh.primitives.size(); primindex++) {
			String meshname = getMeshName(model, meshindex, primindex);
			auto& primitive = glmesh.primitives[primindex];

		    Vector<VertexData> vertices;
			Vector<IndexType> indices;
			extractGLTFIndices(primitive, model, indices);
			extractGLTFVertices(primitive, model, vertices);

			MeshPart part{};
			part.indices = indices;

			Bytes vs;
			vs.resize(vertices.size() * sizeof(decltype(vertices)::value_type));
			memcpy(vs.data(), vertices.data(), vs.size());
			part.vertexData = vs;
			part.numVertices = static_cast<uint32_t>(vertices.size());
			mesh->addPart(std::move(part));
		}
    }

	return mesh;
}
