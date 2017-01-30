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

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

		class TypedDB
		{
		public:
			void add(const String& name, Entry&& asset);
			const Entry& get(const String& name);

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

		private:
			HashMap<String, Entry> assets;
		};

		void addAsset(const String& name, AssetType type, Entry&& entry);
		const TypedDB& getDatabase(AssetType type) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		mutable TreeMap<int, TypedDB> dbs;
	};
}
