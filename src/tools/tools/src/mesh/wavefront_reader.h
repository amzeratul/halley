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
		
		Vector<VertexData> vertices;
		Vector<IndexType> indices;
		
		Vector<Vector3f> v;
		Vector<Vector3f> vt;
		Vector<Vector3f> vn;

		std::map<FaceVertex, IndexType> vertexMap;

		std::unique_ptr<Mesh> makeMesh();
		void parseLine(const String& data);

		void parseV(Vector<String>& tokens);
		void parseVN(Vector<String>& tokens);
		void parseVT(Vector<String>& tokens);
		void parseF(Vector<String>& tokens);
		void makeTriangle(const FaceVertex& a, const FaceVertex& b, const FaceVertex& c);
		IndexType getIndex(const FaceVertex& vert);
	};
}
