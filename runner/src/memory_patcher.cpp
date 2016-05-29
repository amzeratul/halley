#include "memory_patcher.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>

using namespace Halley;

void MemoryPatchingMappings::generate(const std::vector<DebugSymbol>& prev, const std::vector<DebugSymbol>& next)
{
	struct Mapping
	{
		void* from = nullptr;
		void* to = nullptr;
	};

	// Construct mapping
	std::unordered_map<std::string, Mapping> mapping;
	for (const auto& p: prev) {
		mapping[p.name].from = p.address;
	}
	for (const auto& n : next) {
		mapping[n.name].to = n.address;
	}

	// Flatten
	std::vector<Mapping> flatMap;
	for (auto& kv: mapping) {
		const auto& m = kv.second;
		if (m.from != nullptr && m.from != m.to) {
			flatMap.push_back(m);
		}
	}
	mapping.clear();

	// Sort by from address
	std::sort(flatMap.begin(), flatMap.end(), [](const Mapping& a, const Mapping& b) -> bool { return a.from < b.from; });

	// Copy to src and dst arrays
	minSrc = reinterpret_cast<void*>(-1);
	maxSrc = nullptr;
	for (const auto& m: flatMap) {
		minSrc = std::min(minSrc, m.from);
		maxSrc = std::max(maxSrc, m.from);
		src.push_back(m.from);
		dst.push_back(m.to);
	}

	std::cout << "Generated " << src.size() << " memory re-mappings. From " << prev.size() << " to " << next.size() << "." << std::endl;
}

void MemoryPatcher::patch(MemoryPatchingMappings& mappings)
{
	// TODO
}
