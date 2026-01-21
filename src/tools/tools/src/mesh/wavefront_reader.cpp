#include "wavefront_reader.h"

#include "halley/plugin/iasset_importer.h"
using namespace Halley;

std::unique_ptr<Mesh> WavefrontReader::parse(const Path& filename, const Bytes& data, IAddionalFileReader& reader)
{
	auto state = State(filename, reader);

	// TODO: improve this?
	const auto strData = String(reinterpret_cast<const char*>(data.data()), data.size());
	for (auto& line: strData.split('\n')) {
		state.parseLine(line);
	}
	
	return state.makeMesh();
}


WavefrontReader::State::State(Path filename, IAddionalFileReader& additionalFileReader)
	: filename(std::move(filename))
	, addionalFileReader(&additionalFileReader)
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

	IndexType tryParseIndex(const String& str)
	{
		if (!str.isEmpty() && str.isInteger()) {
			return static_cast<IndexType>(str.toInteger());
		}
		return 0;
	}

	Colour4f tryParseColour(gsl::span<const String> tokens)
	{
		if (tokens.size() == 1) {
			return Colour4f(tryParseFloat(tokens, 0));
		} else if (tokens.size() == 2) {
			return Colour4f(tryParseFloat(tokens, 0), tryParseFloat(tokens, 1), 0);
		} else if (tokens.size() == 3) {
			return Colour4f(tryParseFloat(tokens, 0), tryParseFloat(tokens, 1), tryParseFloat(tokens, 2));
		} else if (tokens.size() >= 3) {
			return Colour4f(tryParseFloat(tokens, 0), tryParseFloat(tokens, 1), tryParseFloat(tokens, 2), tryParseFloat(tokens, 3));
		} else {
			return {};
		}
	}
}

void WavefrontReader::State::resetObject()
{
	vertices.clear();
	indices.clear();
	vertexMap.clear();
	name = {};
	materialName = {};
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
	indices.push_back(getIndex(b));
	indices.push_back(getIndex(c));
}

IndexType WavefrontReader::State::getIndex(const FaceVertex& vert)
{
	const auto iter = vertexMap.find(vert);
	if (iter != vertexMap.end()) {
		return iter->second;
	} else {
		const auto pos = vert.v > 0 ? v.at(vert.v - 1) : Vector3f();
		const auto normal = vert.vn > 0 ? vn.at(vert.vn - 1) : Vector3f();
		const auto tex = vert.vt > 0 ? vt.at(vert.vt - 1) : Vector3f();
		const auto idx = static_cast<IndexType>(vertices.size());

		vertices.emplace_back(MeshObject::VertexData {
			Vector4f(pos.x, pos.y, -pos.z, 1.0f),
			Vector4f(normal.x, normal.y, -normal.z, 1.0f),
			Vector4f(1, 1, 1, 1),
			Vector4f(tex.x, tex.y, tex.z, 0.0f)
		});
		vertexMap[vert] = idx;

		return idx;
	}
}

void WavefrontReader::State::setMaterialLib(gsl::span<const String> tokens)
{
	if (!tokens.empty() && !tokens[0].isEmpty()) {
		parseMaterialLibrary(filename.parentPath() / tokens[0]);
	}
}

void WavefrontReader::State::useMaterial(gsl::span<const String> tokens)
{
	materialName = !tokens.empty() ? tokens[0] : "";
}

void WavefrontReader::State::parseMaterialLibrary(const Path& path)
{
	auto bytes = addionalFileReader->readAdditionalFile(path);
	if (bytes.empty()) {
		Logger::logError("Unable to load material library: " + path.getString());
		return;
	}

	const auto strData = String(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	for (auto& line: strData.split('\n')) {
		parseMtlLine(line);
	}
	finishMaterial();
}

void WavefrontReader::State::parseMtlLine(const String& line)
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

		parseMtlCommand(cmd, tokens.const_span().subspan(1));
	}
}

void WavefrontReader::State::parseMtlCommand(const String& cmd, gsl::span<const String> args)
{
	if (cmd == "newmtl") {
		finishMaterial();
		curMaterial = WFMaterial();
		curMaterial.name = args[0];
	} else if (cmd == "map_Kd") {
		curMaterial.texDiffuse = args[0];
	} else if (cmd == "Ka") {
		curMaterial.colAmbient = tryParseColour(args);
	} else if (cmd == "Kd") {
		curMaterial.colDiffuse = tryParseColour(args);
	} else if (cmd == "Ke") {
		curMaterial.colEmissive = tryParseColour(args);
	} else if (cmd == "Ks") {
		curMaterial.colSpecular = tryParseColour(args);
	} else if (cmd == "Ns") {
		curMaterial.specularExponent = tryParseFloat(args, 0);
	} else if (cmd == "d") {
		curMaterial.alpha = tryParseFloat(args, 0);
	} else if (cmd == "Tr") {
		curMaterial.alpha = 1.0f - tryParseFloat(args, 0);
	} else if (cmd == "Tf") {
		curMaterial.colTransmissivity = tryParseColour(args);
	}
}

void WavefrontReader::State::finishMaterial()
{
	if (!curMaterial.name.isEmpty()) {
		auto name = curMaterial.name; // Copy is intentional due to move below
		materials[std::move(name)] = std::move(curMaterial);
	}
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
}

MeshObject WavefrontReader::State::makeMeshObject()
{
	auto result = MeshObject(std::move(name));
	result.setIndices(std::move(indices));
	result.setVertices(std::move(vertices));

	auto matIter = materials.find(materialName);
	if (matIter != materials.end()) {
		const auto& matDef = matIter->second;
		result.setMaterialName("Halley/StandardMesh"); // TODO?
		if (!matDef.texDiffuse.isEmpty()) {
			result.setTextureNames({ matDef.texDiffuse });
		}
		result.setMaterialParams(matDef.getParams());
	} else {
		if (!materialName.isEmpty()) {
			Logger::logWarning("Material not found while importing WaveFront Obj: " + materialName);
		}
		result.setMaterialName("Halley/StandardMesh");
	}

	resetObject();

	return result;
}

WavefrontReader::FaceVertex::FaceVertex(const String& str)
{
	auto tokens = str.split('/');
	v = tryParseIndex(tokens.at(0));
	vt = tryParseIndex(tokens.at(1));
	vn = tryParseIndex(tokens.at(2));
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


ConfigNode WavefrontReader::WFMaterial::getParams() const
{
	ConfigNode result;
	result["u_colAmbient"] = colAmbient.toVector4();
	result["u_colDiffuse"] = colDiffuse.toVector4();
	result["u_colSpecular"] = colSpecular.toVector4();
	result["u_colEmissive"] = colEmissive.toVector4();
	result["u_colTransmissivity"] = colTransmissivity.withAlpha(alpha).toVector4();
	result["u_specularExponent"] = specularExponent;
	return result;
}
