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

namespace Halley {
	template <typename T, size_t blockLen = 16384>
	class MappedPool {
		struct Entry {
			std::array<char, sizeof(T)> data;
			unsigned short index;
			unsigned short revision;
		};

		struct Block {
			std::vector<Entry> data;

			Block(size_t n)
				: data(blockLen)
			{
				size_t base = n * blockLen;
				for (size_t i = 0; i < blockLen - 1; i++) {
					data[i].index = static_cast<unsigned short>(i + 1 + base);
					data[i].revision = 0;
				}
				data[blockLen - 1].index = 0xFFFF;
				data[blockLen - 1].revision = 0;
			}
		};

	public:
		std::pair<T*, int> alloc() {
			if (next == 0xFFFF) {
				blocks.push_back(Block(blocks.size()));
				next = static_cast<unsigned short>((blocks.size() - 1) * blockLen);
			}
			int blockIdx = next;
			auto& block = blocks[blockIdx / blockLen];
			int localIdx = blockIdx % blockLen;
			auto& data = block.data[localIdx];
			int rev = data.revision;
			T* result = reinterpret_cast<T*>(&(data.data));

			// Next block is what was stored on the index
			// Index is now used to store the external index of the current block, so update that
			std::swap(next, block.data[localIdx].index);

			int externalIdx = static_cast<signed int>(blockIdx & 0xFFFF) | (static_cast<signed int>(rev & 0x7FFFF) << 16); // TODO: compute properly
			return std::pair<T*, int>(result, externalIdx);
		}

		void free(T* p) {
			Entry* entry = reinterpret_cast<Entry*>(p);
			std::swap(entry->index, next);
			++entry->revision;
		}

		void freeId(int externalIdx) {
			free(get(externalIdx));
		}

		T* get(int externalIdx) {
			unsigned short idx = static_cast<unsigned short>(externalIdx & 0xFFFF);
			unsigned short rev = static_cast<unsigned short>(externalIdx >> 16);

			int blockN = idx / blockLen;
			if (blockN < 0 || blockN >= signed(blocks.size())) {
				return nullptr;
			}
			auto& block = blocks[blockN];
			int localIdx = idx % blockLen;
			auto& data = block.data[localIdx];
			if (data.revision != rev) {
				return nullptr;
			}
			return reinterpret_cast<T*>(&(data.data));
		}

	private:
		std::vector<Block> blocks;
		unsigned short next = 0xFFFF;
	};
}
