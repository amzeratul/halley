#include "wavefront_reader.h"
using namespace Halley;

std::unique_ptr<Mesh> WavefrontReader::parse(const Bytes& data, IAddionalFileReader& reader)
{
	auto state = State(reader);

	// TODO: improve this?
	const auto strData = String(reinterpret_cast<const char*>(data.data()), data.size());
	for (auto& line: strData.split('\n')) {
		state.parseLine(line);
	}
	
	return state.makeMesh();
}

WavefrontReader::State::State(IAddionalFileReader& additionalFileReader)
	: addionalFileReader(&additionalFileReader)
{
}

std::unique_ptr<Mesh> WavefrontReader::State::makeMesh()
{
	finishObject();
	return std::make_unique<Mesh>(std::move(meshObjects));
}

void WavefrontReader::State::parseLine(const String& line)
{
	auto cleanLine = line.replaceAll("  ", " ");
	cleanLine.trimBoth();

	if (cleanLine.startsWith("#")) {
		return;
	}

	auto tokens = cleanLine.split(' ');

	if (!tokens.empty()) {
		const auto& cmd = tokens[0];
		if (cmd.isEmpty()) {
			return;
		}

		const auto args = tokens.const_span().subspan(1);

		if (cmd == "v") {
			parseV(args);
		} else if (cmd == "vn") {
			parseVN(args);
		} else if (cmd == "vt") {
			parseVT(args);
		} else if (cmd == "f") {
			parseF(args);
		} else if (cmd == "o") {
			startObject(args);
		} else if (cmd == "mtllib") {
			setMaterialLib(args);
		} else if (cmd == "usemtl") {
			useMaterial(args);
		} else {
			Logger::logWarning("Unknown command in Wavefront Obj file: " + cmd, true);
		}
	}
}

namespace {
	float tryParseFloat(gsl::span<const String> tokens, size_t idx)
	{
		if (tokens.size() > idx) {
			return tokens[idx].toFloat();
		} else {
			return 0.0f;
		}
	}
}

void WavefrontReader::State::resetObject()
{
	vertices.clear();
	indices.clear();
	vertexMap.clear();
	name = {};
	material = {};
}

void WavefrontReader::State::parseV(gsl::span<const String> tokens)
{
	v.emplace_back(Vector3f(tryParseFloat(tokens, 0), tryParseFloat(tokens, 1), tryParseFloat(tokens, 2)));
}

void WavefrontReader::State::parseVN(gsl::span<const String> tokens)
{
	vn.emplace_back(Vector3f(tryParseFloat(tokens, 0), tryParseFloat(tokens, 1), tryParseFloat(tokens, 2)));
}

void WavefrontReader::State::parseVT(gsl::span<const String> tokens)
{
	vt.emplace_back(Vector3f(tryParseFloat(tokens, 0), 1.0f - tryParseFloat(tokens, 1), tryParseFloat(tokens, 2)));
}

void WavefrontReader::State::parseF(gsl::span<const String> tokens)
{
	auto a = FaceVertex(tokens[0]);
	auto b = FaceVertex(tokens[1]);
	auto c = FaceVertex(tokens[2]);
	makeTriangle(a, b, c);
	if (tokens.size() == 4 && !tokens[3].isEmpty()) {
		auto d = FaceVertex(tokens[3]);
		makeTriangle(c, d, a);
	}
}

void WavefrontReader::State::makeTriangle(const FaceVertex& a, const FaceVertex& b, const FaceVertex& c)
{
	indices.push_back(getIndex(a));
	indices.push_back(getIndex(c));
	indices.push_back(getIndex(b));
}

IndexType WavefrontReader::State::getIndex(const FaceVertex& vert)
{
	const auto iter = vertexMap.find(vert);
	if (iter != vertexMap.end()) {
		return iter->second;
	} else {
		const auto& pos = v.at(vert.v - 1);
		const auto& normal = vn.at(vert.vn - 1);
		const auto& tex = vt.at(vert.vt - 1);
		const auto idx = static_cast<IndexType>(vertices.size());

		vertices.emplace_back(VertexData{
			Vector4f(pos.x, pos.y, pos.z, 1.0f),
			Vector4f(normal.x, normal.y, normal.z, 1.0f),
			Vector4f(1, 1, 1, 1),
			Vector4f(tex.x, tex.y, tex.z, 0.0f)
		});
		vertexMap[vert] = idx;

		return idx;
	}
}

void WavefrontReader::State::setMaterialLib(gsl::span<const String> tokens)
{
	materialLibrary = !tokens.empty() ? tokens[0] : "";
	// TODO: parse material library
}

void WavefrontReader::State::useMaterial(gsl::span<const String> tokens)
{
	material = !tokens.empty() ? tokens[0] : "";
	// TODO: parse material
}

void WavefrontReader::State::startObject(gsl::span<const String> tokens)
{
	finishObject();
	name = !tokens.empty() ? tokens[0] : "";
}

void WavefrontReader::State::finishObject()
{
	if (!vertices.empty()) {
		meshObjects += makeMeshObject();
	}
	resetObject();
}

MeshObject WavefrontReader::State::makeMeshObject()
{
	auto result = MeshObject(name);

	result.setIndices(std::move(indices));

	Bytes vs;
	vs.resize(vertices.size() * sizeof(decltype(vertices)::value_type));
	memcpy(vs.data(), vertices.data(), vs.size());
	result.setVertices(vertices.size(), vs);

	result.setMaterialName("Halley/StandardMesh"); // TODO
	result.setTextureNames({"texture/meshTexture0.jpg"}); // TODO

	return result;
}

WavefrontReader::FaceVertex::FaceVertex(const String& str)
{
	auto tokens = str.split('/');
	v = static_cast<IndexType>(tokens.at(0).toInteger());
	vt = static_cast<IndexType>(tokens.at(1).toInteger());
	vn = static_cast<IndexType>(tokens.at(2).toInteger());
}

bool WavefrontReader::FaceVertex::operator<(const FaceVertex& other) const
{
	if (v != other.v) {
		return v < other.v;
	}
	if (vt != other.vt) {
		return vt < other.vt;
	}
	return vn < other.vn;
}
