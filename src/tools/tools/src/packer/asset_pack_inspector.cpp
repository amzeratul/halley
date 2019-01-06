#include "halley/tools/packer/asset_pack_inspector.h"
#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/resources/asset_pack.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/compression.h"
#include "halley/support/console.h"
#include "halley/core/resources/asset_database.h"

using namespace Halley;

AssetPackInspector::AssetPackInspector(String name)
	: name(name)
{
	
}

void AssetPackInspector::parse(const Bytes& bytes)
{
	auto s = Deserializer(bytes);
	AssetPackHeader header;
	auto headerSpan = gsl::as_writeable_bytes(gsl::span<AssetPackHeader>(&header, 1));
	s >> headerSpan;

	Bytes tableData(header.dataStartPos - header.assetDbStartPos);
	auto tableSpan = gsl::as_writeable_bytes(gsl::span<Byte>(tableData.data(), tableData.size()));
	s >> tableSpan;

	rawTableSize = tableData.size();
	auto rawTableData = Compression::decompress(tableData);
	tableSize = rawTableData.size();
	parseTable(std::move(rawTableData));
}

void AssetPackInspector::parseTable(Bytes bytes)
{
	auto s = Deserializer(bytes);
	unsigned int numTypedDbs;
	s >> numTypedDbs;

	for (unsigned int i = 0; i < numTypedDbs; ++i) {
		parseTypedDB(s);
	}
}

void AssetPackInspector::parseTypedDB(Deserializer& s)
{
	int curAssetType;
	s >> curAssetType;

	unsigned int numEntries;
	s >> numEntries;

	entries.reserve(entries.size() + numEntries);

	for (unsigned int i = 0; i < numEntries; ++i) {
		String key;
		AssetDatabase::Entry entry;
		s >> key >> entry;

		entries.emplace_back(curAssetType, std::move(key), std::move(entry));
	}
}

void AssetPackInspector::printData() const
{
	auto stdCol = ConsoleColour();
	auto infoCol = ConsoleColour(Console::MAGENTA);
	auto strCol = ConsoleColour(Console::DARK_GREY);
	std::cout << "Pack " << strCol << name << stdCol << "\n";
	std::cout << "  Table size: " << infoCol << rawTableSize << stdCol << " -> " << infoCol << tableSize << stdCol << "\n";

	int lastType = -1;
	int i = -1;
	for (auto& entry: entries) {
		if (entry.assetType != lastType) {
			lastType = entry.assetType;
			i = 0;

			std::cout << "  Assets of type " << infoCol << lastType << stdCol << ":\n";
		}

		auto splitPath = entry.entry.path.split(':');
		std::cout << "    [" << i << "] " << strCol << entry.key << stdCol << ": at " << infoCol << splitPath.at(0) << stdCol << ", " << infoCol << splitPath.at(1) << stdCol << " bytes, " << strCol << toString(entry.entry.meta) <<  stdCol << "\n";

		++i;
	}

	std::cout << std::endl;
}

AssetPackInspector::Entry::Entry(int assetType, String key, AssetDatabase::Entry entry)
	: assetType(assetType)
	, key(std::move(key))
	, entry(std::move(entry))
{
}

int AssetPackInspectorTool::run(Vector<std::string> args)
{
	try {
		if (!args.empty()) {
			for (auto& arg: args) {
				auto data = FileSystem::readFile(arg);
				AssetPackInspector inspector(arg);
				inspector.parse(data);
				inspector.printData();
			}

			return 0;
		} else {
			Logger::logError("Usage: halley-cmd pack-inspector path/to/pack1.dat [path/to/pack2.dat ...]");
			return 1;
		}
	} catch (std::exception& e) {
		Logger::logException(e);
		return 1;
	} catch (...) {
		Logger::logError("Unknown exception packing files.");
		return 1;
	}
}
