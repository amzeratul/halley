#include "halley/tools/packer/asset_pack_inspector.h"
#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/resources/asset_pack.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/compression.h"
#include "halley/support/console.h"
#include "halley/core/resources/asset_database.h"
#include "halley/utils/hash.h"

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
	dataStartPos = header.dataStartPos;

	Bytes tableData(header.dataStartPos - header.assetDbStartPos);
	auto tableSpan = gsl::as_writeable_bytes(gsl::span<Byte>(tableData.data(), tableData.size()));
	s >> tableSpan;

	rawTableSize = tableData.size();
	auto rawTableData = Compression::decompress(tableData);
	tableSize = rawTableData.size();
	parseTable(Deserializer(rawTableData), bytes);

	// Generated sorted entries
	sortedEntries.resize(entries.size());
	for (size_t i = 0; i < sortedEntries.size(); ++i) {
		sortedEntries[i] = int(i);
	}
	std::sort(sortedEntries.begin(), sortedEntries.end(), [&] (int a, int b) -> bool
	{
		return entries[a].key < entries[b].key;
	});

	computeHash();
}

void AssetPackInspector::parseTable(Deserializer s, const Bytes& packBytes)
{
	unsigned int numTypedDbs;
	s >> numTypedDbs;

	for (unsigned int i = 0; i < numTypedDbs; ++i) {
		parseTypedDB(s, packBytes);
	}
}

void AssetPackInspector::parseTypedDB(Deserializer& s, const Bytes& packBytes)
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

		auto splitPath = entry.path.split(':');
		size_t pos = splitPath.at(0).toInteger64();
		size_t size = splitPath.at(1).toInteger64();
		auto hash = Hash::hash(gsl::as_bytes(gsl::span<const Byte>(packBytes.data() + pos + dataStartPos, size)));

		entries.emplace_back(curAssetType, hash, std::move(key), std::move(entry));
	}
}

void AssetPackInspector::computeHash()
{
	Hash::Hasher hasher;
	for (auto& e: sortedEntries) {
		auto& entry = entries[e];
		hasher.feedBytes(gsl::as_bytes(gsl::span<const char>(entry.key.c_str(), entry.key.length())));
		hasher.feed(entry.assetType);
		hasher.feed(entry.hash);
	}
	totalHash = hasher.digest();
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
		std::cout << "    [" << i << "] " << strCol << entry.key << stdCol << " [" << infoCol << toString(entry.hash, 16) << stdCol << "]: at " << infoCol << splitPath.at(0) << stdCol << ", " << infoCol << splitPath.at(1) << stdCol << " bytes, " << strCol << toString(entry.entry.meta) <<  stdCol << "\n";

		++i;
	}

	std::cout << "Hash: " << infoCol << toString(totalHash, 16) << stdCol << "\n";

	std::cout << std::endl;
}

AssetPackInspector::Entry::Entry(int assetType, uint64_t hash, String key, AssetDatabase::Entry entry)
	: assetType(assetType)
	, hash(hash)
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
