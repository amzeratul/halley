#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector4.h"
#include "../graphics_enums.h"
#include "halley/graphics/texture_descriptor.h"

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

	struct MeshPart
	{
		uint32_t numVertices = 0;
		Bytes vertexData;
		Vector<IndexType> indices;

		String materialName;
		Vector<String> textureNames;
		std::shared_ptr<Material> material;

		void serialize(Serializer& serializer) const;
		void deserialize(Deserializer& deserializer, ResourceLoader& loader);
	};

    class Mesh final : public Resource {
    public:
		Mesh();
		explicit Mesh(ResourceLoader& loader);

		static std::unique_ptr<Mesh> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Mesh; }

		void addPart(MeshPart part);
		const Vector<MeshPart>& getParts() const;

		void serialize(Serializer& serializer) const;
		void deserialize(Deserializer& deserializer, ResourceLoader& loader);

    private:
		uint32_t count = 0;
		Vector<MeshPart> meshParts;
    };
}
