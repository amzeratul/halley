#pragma once
#include "../tool/cli_tool.h"
#include "font_face.h"

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
		void generateYAML(String imgName, FontFace& font, std::vector<CharcodeEntry>& entries, String outPath, float scale);
	};
}
