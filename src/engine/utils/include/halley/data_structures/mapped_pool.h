#pragma once

/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include <cstdint>

namespace Halley {
	template <typename T, size_t blockLen = 16384>
	class MappedPool {
		struct Entry {
			alignas(T) std::array<char, sizeof(T)> data;
			uint32_t nextFreeEntryIndex;
			uint32_t revision;
		};

		struct Block {
			Vector<Entry> data;

			Block(size_t blockIndex)
				: data(blockLen)
			{
				size_t base = blockIndex * blockLen;
				for (size_t i = 0; i < blockLen - 1; i++) {
					// Each entry points to the next
					data[i].nextFreeEntryIndex = static_cast<uint32_t>(i + 1 + base);
					data[i].revision = 0;
				}
				data[blockLen - 1].nextFreeEntryIndex = 0xFFFF;
				data[blockLen - 1].revision = 0;
			}
		};

	public:
		std::pair<T*, int64_t> alloc() {
			// Next entry will be at position "entryIdx", which is just what was stored on next
			int entryIdx = next;

			// Figure which block it goes into, and make sure that exists
			int blockIdx = entryIdx / blockLen;
			if (blockIdx >= blocks.size()) {
				blocks.push_back(Block(blocks.size()));
			}
			auto& block = blocks[blockIdx];

			// Find the local entry inside that block and initialize it
			int localIdx = entryIdx % blockLen;
			auto& data = block.data[localIdx];
			int rev = data.revision;
			T* result = reinterpret_cast<T*>(&(data.data));

			// Next block is what was stored on the nextFreeEntryIndex
			std::swap(next, block.data[localIdx].nextFreeEntryIndex);

			// External index composes the revision with the index, so it's unique, but easily mappable
			int64_t externalIdx = static_cast<int64_t>(entryIdx) | (static_cast<int64_t>(rev & 0x7FFFFFFF) << 32); // TODO: compute properly
			return std::pair<T*, int64_t>(result, externalIdx);
		}

		void free(T* p) {
			// Swaps the data with the next, so this will actually be the next one to be allocated
			Entry* entry = reinterpret_cast<Entry*>(p);
			std::swap(entry->nextFreeEntryIndex, next);

			// Increase revision so the next one to allocate this gets a unique number
			++entry->revision;
		}

		void freeId(int64_t externalIdx) {
			free(get(externalIdx));
		}

		T* get(int64_t externalIdx) {
			auto idx = static_cast<uint32_t>(externalIdx & 0xFFFFFFFFll);
			auto rev = static_cast<uint32_t>(externalIdx >> 32);

			int blockN = idx / blockLen;
			if (blockN < 0 || blockN >= int(blocks.size())) {
				return nullptr;
			}

			// TODO: check if can shrink?

			auto& block = blocks[blockN];
			int localIdx = idx % blockLen;
			auto& data = block.data[localIdx];
			if (data.revision != rev) {
				return nullptr;
			}
			return reinterpret_cast<T*>(&(data.data));
		}

	private:
		Vector<Block> blocks;
		uint32_t next = 0;
	};
}
