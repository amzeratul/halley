#pragma once
#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include "halley/tools/cli_tool.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/asset_database.h"

namespace Halley {
    class AssetPackInspector {
    public:
	    explicit AssetPackInspector(String name);
	    void parse(const Bytes& bytes);
	    void printData() const;

    private:
		String name;
		size_t rawTableSize;
		size_t tableSize;
		uint64_t totalHash;
	    uint64_t dataStartPos;

	    struct Entry
		{
			int assetType;
			uint64_t hash;
			String key;
			AssetDatabase::Entry entry;

			Entry(int assetType, uint64_t hash, String key, AssetDatabase::Entry entry);
		};
		std::vector<Entry> entries;
		std::vector<int> sortedEntries;

		void parseTable(Deserializer s, const Bytes& packBytes);
	    void parseTypedDB(Deserializer& s, const Bytes& packBytes);
		void computeHash();
    };

	class AssetPackInspectorTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;
	};
}
