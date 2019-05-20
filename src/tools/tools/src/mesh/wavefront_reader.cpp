#include "wavefront_reader.h"
using namespace Halley;

std::unique_ptr<Mesh> WavefrontReader::parse(const Bytes& data)
{
	// TODO: improve this
	const auto strData = String(reinterpret_cast<const char*>(data.data()), data.size());
	for (auto& line: strData.split('\n')) {
		parseLine(line);
	}

	return makeMesh();
}

std::unique_ptr<Mesh> WavefrontReader::makeMesh()
{
	auto result = std::make_unique<Mesh>();
	result->setIndices(std::move(indices));
	Bytes vs;
	vs.resize(vertices.size() * sizeof(decltype(vertices)::value_type));
	memcpy(vs.data(), vertices.data(), vs.size());
	result->setVertices(vertices.size(), vs);
	result->setMaterialName("Halley/StandardMesh"); // TODO
	result->setTextureNames({"texture/meshTexture0.jpg"}); // TODO
	return result;
}

void WavefrontReader::parseLine(const String& line)
{
	auto cleanLine = line.replaceAll("  ", " ");
	cleanLine.trimBoth();

	if (cleanLine.startsWith("#")) {
		return;
	}

	auto tokens = cleanLine.split(' ');

	if (!tokens.empty()) {
		if (tokens[0] == "v") {
			parseV(tokens);
		} else if (tokens[0] == "vn") {
			parseVN(tokens);
		} else if (tokens[0] == "vt") {
			parseVT(tokens);
		} else if (tokens[0] == "f") {
			parseF(tokens);
		}
	}
}

void WavefrontReader::parseV(std::vector<String>& tokens)
{
	v.emplace_back(Vector3f(tokens.at(1).toFloat(), tokens.at(2).toFloat(), tokens.at(3).toFloat()));
}

void WavefrontReader::parseVN(std::vector<String>& tokens)
{
	vn.emplace_back(Vector3f(tokens.at(1).toFloat(), tokens.at(2).toFloat(), tokens.at(3).toFloat()));
}

void WavefrontReader::parseVT(std::vector<String>& tokens)
{
	vt.emplace_back(Vector3f(tokens.at(1).toFloat(), 1.0f - tokens.at(2).toFloat(), tokens.at(3).toFloat()));
}

void WavefrontReader::parseF(std::vector<String>& tokens)
{
	auto a = FaceVertex(tokens[1]);
	auto b = FaceVertex(tokens[2]);
	auto c = FaceVertex(tokens[3]);
	makeTriangle(a, b, c);
	if (tokens.size() == 5 && !tokens[4].isEmpty()) {
		auto d = FaceVertex(tokens[4]);
		makeTriangle(c, d, a);
	}
}

void WavefrontReader::makeTriangle(const FaceVertex& a, const FaceVertex& b, const FaceVertex& c)
{
	indices.push_back(getIndex(a));
	indices.push_back(getIndex(c));
	indices.push_back(getIndex(b));
}

IndexType WavefrontReader::getIndex(const FaceVertex& vert)
{
	const auto iter = vertexMap.find(vert);
	if (iter != vertexMap.end()) {
		return iter->second;
	} else {
		const auto& pos = v.at(vert.v - 1);
		const auto& normal = vn.at(vert.vn - 1);
		const auto& tex = vt.at(vert.vt - 1);
		const auto idx = IndexType(vertices.size());
		vertices.emplace_back(VertexData{ Vector4f(pos.x, pos.y, pos.z, 1.0f), Vector4f(normal.x, normal.y, normal.z, 1.0f), Vector4f(1, 1, 1, 1), Vector4f(tex.x, tex.y, tex.z, 0.0f) });
		vertexMap[vert] = idx;
		return idx;
	}
}

WavefrontReader::FaceVertex::FaceVertex()
{}

WavefrontReader::FaceVertex::FaceVertex(const String& str)
{
	auto tokens = str.split('/');
	v = tokens.at(0).toInteger();
	vt = tokens.at(1).toInteger();
	vn = tokens.at(2).toInteger();
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
