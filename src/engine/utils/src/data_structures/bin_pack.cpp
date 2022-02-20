#include "halley/data_structures/bin_pack.h"

#ifdef _MSC_VER
#pragma warning(disable: 4456 4458)
#endif
#include "binpack2d.hpp"
#include <queue>
#include "halley/support/logger.h"

using namespace Halley;

std::optional<Vector<BinPackResult>> BinPack::pack(const Vector<BinPackEntry>& entries, Vector2i binSize)
{
	using T = void*;

	BinPack2D::ContentAccumulator<T> inputContent;
	for (auto& e: entries) {
		inputContent += BinPack2D::Content<T>(e.data, BinPack2D::Coord(), BinPack2D::Size(e.size.x, e.size.y), false);
	}
	inputContent.Sort();
	BinPack2D::CanvasArray<T> canvasArray = BinPack2D::UniformCanvasArrayBuilder<T>(binSize.x, binSize.y, 1).Build();
	BinPack2D::ContentAccumulator<T> remainder;
	bool success = canvasArray.Place(inputContent, remainder);
	BinPack2D::ContentAccumulator<T> outputContent;
	canvasArray.CollectContent(outputContent);

	if (success) {
		Vector<BinPackResult> results;
		for (auto& content: outputContent.Get()) {
			results.push_back(BinPackResult(Rect4i(content.coord.x, content.coord.y, content.size.w, content.size.h), content.rotated, content.content));
		}
		return std::optional<Vector<BinPackResult>>(std::move(results));
	} else {
		return std::optional<Vector<BinPackResult>>();
	}
}

std::optional<Vector<BinPackResult>> BinPack::fastPack(const Vector<BinPackEntry>& entries, Vector2i binSize)
{
	Vector<BinPackResult> result;
	std::priority_queue<BinPackEntry> queue;
	for (auto& e: entries) {
		queue.push(e);
	}

	Vector2i cursorPos;
	int lineHeight = 0;
	int binsInLine = 0;

	while (!queue.empty()) {
		auto& next = queue.top();
		auto nextSize = next.size;
		bool rotated = false;
		if (next.canRotate && nextSize.x > nextSize.y) {
			rotated = true;
			std::swap(nextSize.x, nextSize.y);
		}

		const int xFree = binSize.x - cursorPos.x;
		if (nextSize.x > xFree) {
			// Can't fit this line anymore
			if (binsInLine > 0) {
				cursorPos.x = 0;
				cursorPos.y += lineHeight;
				const int yFree = binSize.y - cursorPos.y;
				if (nextSize.y > yFree) {
					// Can't fit, give up
					return {};
				}

				// New line started
				lineHeight = 0;
				binsInLine = 0;
			} else {
				// Ops, can't fit at all, give up
				return {};
			}
		}

		result.push_back(BinPackResult(Rect4i(cursorPos.x, cursorPos.y, nextSize.x, nextSize.y), rotated, next.data));
		cursorPos.x += nextSize.x;
		lineHeight = std::max(lineHeight, nextSize.y);
		++binsInLine;
		queue.pop();
	}

	return result;
}
