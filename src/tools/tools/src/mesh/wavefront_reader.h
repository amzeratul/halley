#pragma once
#include "halley/file/path.h"
#include "halley/graphics/mesh/mesh.h"
#include "halley/maths/colour.h"

namespace Halley
{
	class Path;
	class IAddionalFileReader;

	class WavefrontReader
	{
	public:
		static std::unique_ptr<Mesh> parse(const Path& filename, const Bytes& data, IAddionalFileReader& reader);

	private:
		struct FaceVertex
		{
			FaceVertex() = default;
			FaceVertex(const String& str);

			bool operator<(const FaceVertex& other) const;

			IndexType v = 0;
			IndexType vt = 0;
			IndexType vn = 0;
		};

		class WFMaterial {
		public:
			String name;

			String texDiffuse;

			Colour4f colAmbient = Colour4f(0, 0, 0);
			Colour4f colDiffuse = Colour4f(1, 1, 1);
			Colour4f colSpecular = Colour4f(0, 0, 0);
			Colour4f colEmissive = Colour4f(0, 0, 0);
			Colour4f colTransmissivity = Colour4f(0, 0, 0);

			float alpha = 1.0f;
			float specularExponent = 100;

			ConfigNode getParams() const;
		};

		class State {
		public:
			State(Path filename, IAddionalFileReader& additionalFileReader);

			void parseLine(const String& data);
			std::unique_ptr<Mesh> makeMesh();

		private:
			Vector<MeshObject::VertexData> vertices;
			Vector<IndexType> indices;
			std::map<FaceVertex, IndexType> vertexMap;

			Vector<Vector3f> v;
			Vector<Vector3f> vt;
			Vector<Vector3f> vn;

			String name;
			String materialName;

			WFMaterial curMaterial;
			HashMap<String, WFMaterial> materials;

			Vector<MeshObject> meshObjects;

			Path filename;
			IAddionalFileReader* addionalFileReader = nullptr;

			void resetObject();

			void parseV(gsl::span<const String> tokens);
			void parseVN(gsl::span<const String> tokens);
			void parseVT(gsl::span<const String> tokens);
			void parseF(gsl::span<const String> tokens);
			void makeTriangle(const FaceVertex& a, const FaceVertex& b, const FaceVertex& c);
			IndexType getIndex(const FaceVertex& vert);

			void setMaterialLib(gsl::span<const String> tokens);
			void useMaterial(gsl::span<const String> tokens);
			void parseMaterialLibrary(const Path& path);

			void parseMtlLine(const String& line);
			void parseMtlCommand(const String& cmd, gsl::span<const String> args);
			void finishMaterial();

			void startObject(gsl::span<const String> tokens);
			void finishObject();
			MeshObject makeMeshObject();
		};
	};
}
