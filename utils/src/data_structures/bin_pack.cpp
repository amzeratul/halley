#include "bin_pack.h"

#ifdef _MSC_VER
#pragma warning(disable: 4456 4458)
#endif
#include "binpack2d.hpp"

using namespace Halley;

Maybe<std::vector<BinPackResult>> BinPack::pack(std::vector<BinPackEntry> entries, Vector2i binSize)
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
		std::vector<BinPackResult> results;
		for (auto& content: outputContent.Get()) {
			results.push_back(BinPackResult(Rect4i(content.coord.x, content.coord.y, content.size.w, content.size.h), content.rotated, content.content));
		}
		return Maybe<std::vector<BinPackResult>>(std::move(results));
	} else {
		return Maybe<std::vector<BinPackResult>>();
	}
}
