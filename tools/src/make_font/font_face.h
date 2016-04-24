#pragma once
#include <type_traits>

namespace Halley
{
	class FontFacePimpl;

	class FontFace
	{
	public:
		explicit FontFace(String filename);
		~FontFace();

		void setSize(float size);

		std::vector<int> getCharCodes() const;
		Vector2i getGlyphSize(int charCode);
		
		void drawGlyph(Image& image, int charcode, Rect4i rect2D);

	private:
		std::unique_ptr<FontFacePimpl> pimpl;
	};
}
