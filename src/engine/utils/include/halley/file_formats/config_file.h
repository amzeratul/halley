#pragma once

#include "../../../../entity/include/halley/entity/entity_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"

namespace Halley
{
	class ResourceLoader;

	class ConfigFileSerializationState : public SerializerState {
	public:
		bool storeFilePosition = false;
	};

	class ConfigFile : public Resource
	{
	public:
		ConfigFile();
		explicit ConfigFile(const ConfigFile& other);
		explicit ConfigFile(ConfigNode root);
		ConfigFile(ConfigFile&& other) noexcept;

		ConfigFile& operator=(const ConfigFile& other);
		ConfigFile& operator=(ConfigFile&& other) noexcept;

		ConfigNode& getRoot();
		const ConfigNode& getRoot() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		static std::unique_ptr<ConfigFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::ConfigFile; }

		void reload(Resource&& resource) override;

	protected:
		ConfigNode root;
		bool storeFilePosition = true;

		void updateRoot();
	};

	class ConfigObserver
	{
	public:
		ConfigObserver();
		ConfigObserver(const ConfigNode& node);
		ConfigObserver(const ConfigFile& file);

		const ConfigNode& getRoot() const;
		
		bool needsUpdate() const;
		void update();
		String getAssetId() const;

	private:
		int assetVersion = 0;
		const ConfigFile* file = nullptr;
		const ConfigNode* node = nullptr;
	};
}
