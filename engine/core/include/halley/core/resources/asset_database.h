#pragma once

#include "halley/text/halleystring.h"
#include "halley/data_structures/tree_map.h"
#include "halley/data_structures/hash_map.h"
#include "halley/resources/metadata.h"

namespace Halley
{
	enum class AssetType;

	class AssetDatabase
	{
	public:
		class Entry
		{
		public:
			String path;
			Metadata meta;

			Entry();
			Entry(const String& path, const Metadata& meta);

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

		class TypedDB
		{
		public:
			void add(const String& name, Entry&& asset);
			const Entry& get(const String& name) const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			const HashMap<String, Entry>& getAssets() const;

		private:
			HashMap<String, Entry> assets;
		};

		void addAsset(const String& name, AssetType type, Entry&& entry);
		const TypedDB& getDatabase(AssetType type) const;
		std::vector<String> getAssets() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
		std::vector<String> enumerate(AssetType type) const;

	private:
		mutable TreeMap<int, TypedDB> dbs;
	};
}
