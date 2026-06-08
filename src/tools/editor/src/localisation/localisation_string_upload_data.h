#pragma once
#include "localisation_data.h"

namespace Halley {
	
	enum class LocStringUploadEntryType {
		Added,
		Removed,
		Modified,
		ModifiedMinor,
		Renamed,
		Noop
	};

	template <>
	struct EnumNames<LocStringUploadEntryType> {
		constexpr auto operator()() const {
			return std::to_array({
				"added",
				"removed",
				"modified",
				"modifiedMinor",
				"renamed",
				"noop"
			});
		}
	};

	class LocStringUploadChunkData {
	public:
		struct Entry {
			String key;
			String value;
			std::optional<String> remoteValue;
			std::optional<String> oldKey;
			LocStringUploadEntryType type;
			bool send = true;
		};

		String chunkId;
		Vector<Entry> entries;
		bool isDelete = false;

		LocStringUploadChunkData(String chunkId = "", bool isDelete = false);
		LocStringUploadChunkData(const LocOriginalDataChunk& chunk, const LocOriginalDataChunk* remote = nullptr);

		ConfigNode toConfigNode() const;
	};

	class LocStringUploadData {
	public:
		LocStringUploadData() = default;
		LocStringUploadData(const LocOriginalData& origData, const LocOriginalData& curRemoteData);

		void generate(const LocOriginalData& origData, const LocOriginalData& curRemoteData);

		ConfigNode toConfigNode() const;
		const Vector<LocStringUploadChunkData>& getChunks() const;
		Vector<LocStringUploadChunkData>& getChunks();

	private:
		Vector<LocStringUploadChunkData> chunks;
	};
}
