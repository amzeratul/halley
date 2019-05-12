#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector4.h"
#include "../graphics_enums.h"

namespace Halley {
	class ResourceLoader;
	class Material;

    class Mesh final : public Resource {
    public:
		Mesh();
		explicit Mesh(ResourceLoader& loader);

		static std::unique_ptr<Mesh> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Font; }

        size_t getNumVertices() const;
		gsl::span<const char> getVertexData() const;
		gsl::span<const IndexType> getIndices() const;
        std::shared_ptr<const Material> getMaterial() const;

		void setVertices(size_t num, std::vector<char> vertexData);
		void setIndices(std::vector<IndexType> indices);
		void setMaterialName(String name);
		void setTextureNames(std::vector<String> textureNames);

		void serialize(Serializer& deserializer) const;
		void deserialize(Deserializer& deserializer);

    private:
		size_t numVertices = 0;
        std::vector<char> vertexData;
        std::vector<IndexType> indices;

		String materialName;
		std::vector<String> textureNames;
		std::shared_ptr<Material> material;
    };
}
