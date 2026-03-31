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
		struct Entry {
			String key;
			String value;
			std::optional<String> oldKey;
			LocStringUploadEntryType type;
		};

		String chunkId;
		Vector<Entry> entries;

		LocStringUploadChunkData(String chunkId = "");
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

	private:
		Vector<LocStringUploadChunkData> chunks;
	};
}
