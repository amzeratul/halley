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

		struct Entry
		{
			int assetType;
			String key;
			AssetDatabase::Entry entry;

			Entry(int assetType, String key, AssetDatabase::Entry entry);
		};
		std::vector<Entry> entries;

		void parseTable(Bytes data);
	    void parseTypedDB(Deserializer& s);
    };

	class AssetPackInspectorTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;
	};
}
