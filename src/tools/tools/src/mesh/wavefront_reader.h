#pragma once
#include "halley/core/graphics/mesh/mesh.h"

namespace Halley
{
	class WavefrontReader
	{
	public:
		std::unique_ptr<Mesh> parse(const Bytes& data);

	private:
		struct FaceVertex
		{
			FaceVertex();
			FaceVertex(const String& str);

			bool operator<(const FaceVertex& other) const;

			IndexType v = 0;
			IndexType vt = 0;
			IndexType vn = 0;
		};
		
		std::vector<VertexData> vertices;
		std::vector<IndexType> indices;
		
		std::vector<Vector3f> v;
		std::vector<Vector3f> vt;
		std::vector<Vector3f> vn;

		std::map<FaceVertex, IndexType> vertexMap;

		std::unique_ptr<Mesh> makeMesh();
		void parseLine(const String& data);

		void parseV(std::vector<String>& tokens);
		void parseVN(std::vector<String>& tokens);
		void parseVT(std::vector<String>& tokens);
		void parseF(std::vector<String>& tokens);
		void makeTriangle(const FaceVertex& a, const FaceVertex& b, const FaceVertex& c);
		IndexType getIndex(const FaceVertex& vert);
	};
}
