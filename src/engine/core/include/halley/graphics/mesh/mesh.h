#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector4.h"
#include "../graphics_enums.h"

namespace Halley {
	class ResourceLoader;
	class Material;

    class MeshObject final : public Resource {
    public:
		struct VertexData
		{
			Vector4f pos;
			Vector4f normal;
			Vector4f colour;
			Vector4f texCoord0;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

		MeshObject() = default;
		MeshObject(String name);

    	void loadDependencies(Resources& resources);

    	size_t getNumVertices() const;
		gsl::span<const VertexData> getVertexData() const;
		gsl::span<const IndexType> getIndices() const;
        std::shared_ptr<const Material> getMaterial() const;
		const String& getName() const;

		void setVertices(Vector<VertexData> vertexData);
		void setIndices(Vector<IndexType> indices);
		void setMaterialName(String name);
		void setTextureNames(Vector<String> textureNames);
		void setName(String name);

    	std::pair<Vector3f, Vector3f> getBounds() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
        Vector<VertexData> vertexData;
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

		void loadDependencies(Resources& resources);

		static std::unique_ptr<Mesh> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Mesh; }

		const Vector<MeshObject>& getObjects() const;
    	std::pair<Vector3f, Vector3f> getBounds() const;
		std::pair<Vector3f, Vector3f> getCentreAndSize() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
		Vector<MeshObject> objects;
    };
}
