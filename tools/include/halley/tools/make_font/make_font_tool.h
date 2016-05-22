#pragma once

#include <halley/maths/rect.h>
#include "halley/tools/cli_tool.h"
#include "../../../../src/make_font/font_face.h"

namespace Halley
{
	class MakeFontTool : public CommandLineTool
	{
		struct CharcodeEntry
		{
			int charcode = 0;
			Rect4i rect;

			CharcodeEntry() {}

			CharcodeEntry(int charcode, Rect4i rect)
				: charcode(charcode)
				, rect(rect)
			{}
		};

	public:
		int run(std::vector<std::string> args) override;
		void generateFontMap(String imgName, FontFace& font, std::vector<CharcodeEntry>& entries, String outPath, float scale, float radius);
		void generateTextureMeta(String destination);
	};
}
