#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector4.h"
#include "../graphics_enums.h"

namespace Halley {
	class ResourceLoader;
	class Material;

	struct VertexData
	{
		Vector4f pos;
		Vector4f normal;
		Vector4f colour;
		Vector4f texCoord0;
	};

    class MeshObject final : public Resource {
    public:
		MeshObject() = default;
		MeshObject(String name);

    	void loadDependencies(ResourceLoader& loader);

    	uint32_t getNumVertices() const;
		gsl::span<const Byte> getVertexData() const;
		gsl::span<const IndexType> getIndices() const;
        std::shared_ptr<const Material> getMaterial() const;
		const String& getName() const;

		void setVertices(size_t num, Bytes vertexData);
		void setIndices(Vector<IndexType> indices);
		void setMaterialName(String name);
		void setTextureNames(Vector<String> textureNames);
		void setName(String name);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		uint32_t numVertices = 0;
        Bytes vertexData;
        Vector<IndexType> indices;

		String materialName;
		Vector<String> textureNames;
		std::shared_ptr<Material> material;

		String name;
    };

	class Mesh final : public Resource {
    public:
		Mesh() = default;
		Mesh(Vector<MeshObject> objects);
		explicit Mesh(ResourceLoader& loader);

		static std::unique_ptr<Mesh> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Mesh; }

		const Vector<MeshObject>& getObjects() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
		Vector<MeshObject> objects;
    };
}
