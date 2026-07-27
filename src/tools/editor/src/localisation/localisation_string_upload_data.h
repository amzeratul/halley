#pragma once
#include "localisation_data.h"

namespace Halley {
	
	enum class LocStringUploadEntryType {
		Added,
		Removed,
		Modified,
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
				"renamed",
				"noop"
			});
		}
	};

	class LocStringUploadChunkData {
	public:
		struct DiffEntry {
			String str;
			Vector<std::pair<StringDiffType, size_t>> changeTypes;
		};

		struct Entry {
			String key;
			String value;
			std::optional<String> remoteValue;
			std::optional<String> oldKey;
			std::optional<DiffEntry> valueDiff;
			LocStringUploadEntryType type;
			bool send = true;
			bool minorRevision = false;

			void makeDiff();
		};

		String chunkId;
		Vector<Entry> entries;
		bool isDelete = false;

		LocStringUploadChunkData(String chunkId = "", bool isDelete = false);
		LocStringUploadChunkData(const LocOriginalDataChunk& chunk, const LocOriginalDataChunk* remote = nullptr);

		ConfigNode toConfigNode() const;

		void makeDiff();
	};

	class LocStringUploadData {
	public:
		LocStringUploadData() = default;
		LocStringUploadData(const LocOriginalData& origData, const LocOriginalData& curRemoteData);

		void generate(const LocOriginalData& origData, const LocOriginalData& curRemoteData);

		ConfigNode toConfigNode() const;
		const Vector<LocStringUploadChunkData>& getChunks() const;
		Vector<LocStringUploadChunkData>& getChunks();

		void makeDiff();

	private:
		Vector<LocStringUploadChunkData> chunks;
	};
}
