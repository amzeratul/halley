#include "localisation_string_upload_data.h"

using namespace Halley;

LocStringUploadChunkData::LocStringUploadChunkData(String chunkId)
	: chunkId(std::move(chunkId))
{
}

LocStringUploadChunkData::LocStringUploadChunkData(const LocOriginalDataChunk& chunk, const LocOriginalDataChunk* remote)
{
	chunkId = chunk.name;

	HashMap<String, const LocalisationDataEntry*> myEntries;
	HashMap<String, const LocalisationDataEntry*> remoteEntries;

	if (remote) {
		const auto n = remote->getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			const auto& e = remote->getEntry(i);
			remoteEntries[e.getKey()] = &e;
		}
	}

	// Classify current strings as added, modified, renamed, or no-op
	const auto n = chunk.getNumEntries();
	for (size_t i = 0; i < n; ++i) {
		const auto& e = chunk.getEntry(i);
		myEntries[e.getKey()] = &e;

		LocStringUploadEntryType type = LocStringUploadEntryType::Added;
		if (remote) {
			if (const auto* remoteEntry = remoteEntries.value_or(e.getKey(), nullptr)) {
				type = e.getValue() == remoteEntry->getValue() ? LocStringUploadEntryType::Noop : LocStringUploadEntryType::Modified;
			} else {
				type = LocStringUploadEntryType::Added;
			}
		}
		
		// TODO: detect key renames
		std::optional<String> oldKey;
		if (oldKey) {
			type = LocStringUploadEntryType::Renamed;
		}

		entries += Entry{ e.getKey(), e.getValue(), oldKey, type };
	}
	
	// Check which strings have been removed
	if (remote) {
		const auto n = remote->getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			const auto& e = remote->getEntry(i);
			if (!myEntries.contains(e.getKey())) {
				entries += Entry { e.getKey(), e.getValue(), std::nullopt, LocStringUploadEntryType::Removed };
			}
		}
	}
}

ConfigNode LocStringUploadChunkData::toConfigNode() const
{
	ConfigNode keys;
	ConfigNode values;
	ConfigNode keyRenames;

	for (const auto& entry: entries) {
		if (entry.type != LocStringUploadEntryType::Removed) {
			keys.push_back(ConfigNode(entry.key));
			values.push_back(ConfigNode(entry.value));
		}
		if (entry.oldKey) {
			keyRenames.push_back(ConfigNode(std::pair(*entry.oldKey, entry.key)));
		}
	}

	ConfigNode result;
	result["chunkId"] = chunkId;
	result["keys"] = std::move(keys);
	result["values"] = std::move(values);
	result["keyRenames"] = std::move(keyRenames);
	return result;
}

LocStringUploadData::LocStringUploadData(const LocOriginalData& origData, const LocOriginalData& curRemoteData)
{
	generate(origData, curRemoteData);
}

void LocStringUploadData::generate(const LocOriginalData& origData, const LocOriginalData& curRemoteData)
{
	for (const auto& chunk: origData.getChunks()) {
		if (auto* remote = curRemoteData.tryGetChunk(chunk.name)) {
			// Exists in remote, skip if it's the same
			if (chunk.hasKeyValueChanges(*remote)) {
				chunks.push_back(LocStringUploadChunkData(chunk, remote));
			}
		} else {
			// Doesn't exist in remote, send unless it's empty
			if (chunk.getNumEntries() > 0) {
				chunks.push_back(LocStringUploadChunkData(chunk));
			}
		}
	}

	// Remove stale chunks
	HashSet<String> existingChunks;
	for (const auto& chunk: chunks) {
		existingChunks.insert(chunk.chunkId);
	}
	for (const auto& chunk: curRemoteData.getChunks()) {
		if (!existingChunks.contains(chunk.name)) {
			chunks.push_back(LocStringUploadChunkData(chunk.name));
		}
	}
}

ConfigNode LocStringUploadData::toConfigNode() const
{
	ConfigNode result;
	result["chunks"] = chunks;
	return result;
}

const Vector<LocStringUploadChunkData>& LocStringUploadData::getChunks() const
{
	return chunks;
}
