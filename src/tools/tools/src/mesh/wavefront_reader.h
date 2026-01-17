#pragma once
#include "halley/graphics/mesh/mesh.h"

namespace Halley
{
	class IAddionalFileReader;

	class WavefrontReader
	{
	public:
		static std::unique_ptr<Mesh> parse(const Bytes& data, IAddionalFileReader& reader);

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

		class State {
		public:
			State(IAddionalFileReader& additionalFileReader);

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
			String material;
			String materialLibrary;

			Vector<MeshObject> meshObjects;
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

			void startObject(gsl::span<const String> tokens);
			void finishObject();
			MeshObject makeMeshObject();
		};
	};
}
