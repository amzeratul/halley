#include "localisation_string_upload_data.h"

using namespace Halley;

LocStringUploadChunkData::LocStringUploadChunkData(String chunkId, bool isDelete)
	: chunkId(std::move(chunkId))
	, isDelete(isDelete)
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

		std::optional<String> remoteValue;
		LocStringUploadEntryType type = LocStringUploadEntryType::Added;
		if (remote) {
			if (const auto* remoteEntry = remoteEntries.value_or(e.getKey(), nullptr)) {
				type = e.getValue() == remoteEntry->getValue() ? LocStringUploadEntryType::Noop : LocStringUploadEntryType::Modified;
				remoteValue = remoteEntry->getValue();
			} else {
				type = LocStringUploadEntryType::Added;
			}
		}
		
		// TODO: detect key renames
		std::optional<String> oldKey;
		if (oldKey) {
			type = LocStringUploadEntryType::Renamed;
		}

		entries += Entry{ e.getKey(), e.getValue(), std::move(remoteValue), std::move(oldKey), type, true };
	}
	
	// Check which strings have been removed
	if (remote) {
		const auto n = remote->getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			const auto& e = remote->getEntry(i);
			if (!myEntries.contains(e.getKey())) {
				entries += Entry { e.getKey(), "", e.getValue(), std::nullopt, LocStringUploadEntryType::Removed, true };
			}
		}
	}
}

ConfigNode LocStringUploadChunkData::toConfigNode() const
{
	ConfigNode keys;
	ConfigNode values;
	ConfigNode keysToKeep;
	ConfigNode keyRenames;

	bool hasSend = false;

	for (const auto& entry: entries) {
		if (entry.send) {
			if (entry.type == LocStringUploadEntryType::Noop) {
				keysToKeep.push_back(ConfigNode(entry.key));
			} else {
				hasSend = true;
				if (entry.type != LocStringUploadEntryType::Removed) {
					keys.push_back(ConfigNode(entry.key));
					values.push_back(ConfigNode(entry.value));
				}
			}

			if (entry.oldKey) {
				keyRenames.push_back(ConfigNode(std::pair(*entry.oldKey, entry.key)));
			}
		} else if (entry.remoteValue) {
			keysToKeep.push_back(ConfigNode(entry.key));
		}
	}

	// Don't send this chunk at all if all its keys are untagged to send
	if (!hasSend) {
		return {};
	}

	ConfigNode result;
	result["chunkId"] = chunkId;
	result["keys"] = std::move(keys);
	result["values"] = std::move(values);
	result["keysToKeep"] = std::move(keysToKeep);
	result["keyRenames"] = std::move(keyRenames);
	return result;
}

LocStringUploadData::LocStringUploadData(const LocOriginalData& origData, const LocOriginalData& curRemoteData)
{
	generate(origData, curRemoteData);
}

void LocStringUploadData::generate(const LocOriginalData& origData, const LocOriginalData& curRemoteData)
{
	HashSet<String> existingChunks;
	for (const auto& chunk: origData.getChunks()) {
		existingChunks.insert(chunk.name);
		if (auto* remote = curRemoteData.tryGetChunk(chunk.name)) {
			// Exists in remote, send if changed
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

	std_ex::erase_if(result["chunks"].asSequence(), [] (const ConfigNode& e) {
		return e.getType() == ConfigNodeType::Undefined;
	});

	return result;
}

const Vector<LocStringUploadChunkData>& LocStringUploadData::getChunks() const
{
	return chunks;
}

Vector<LocStringUploadChunkData>& LocStringUploadData::getChunks()
{
	return chunks;
}
