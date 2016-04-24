#include "font_face.h"
#include <ft2build.h>
#include FT_FREETYPE_H

using namespace Halley;

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "freetyped.lib")
#else
#pragma comment(lib, "freetype.lib")
#endif
#endif

namespace Halley {
	class FontFacePimpl
	{
	public:
		FT_Library library = nullptr;
		FT_Face face = nullptr;
	};
}

FontFace::FontFace(String filename)
	: pimpl (std::make_unique<FontFacePimpl>())
{
	int error = 0;

	error = FT_Init_FreeType(&pimpl->library);
	if (error) {
		throw Exception("Unable to initialize FreeType");
	}
	error = FT_New_Face(pimpl->library, filename.c_str(), 0, &pimpl->face);
	if (error) {
		throw Exception("Unable to load font face");
	}
}

FontFace::~FontFace()
{
	if (pimpl->face) {
		FT_Done_Face(pimpl->face);
	}
	if (pimpl->library) {
		FT_Done_FreeType(pimpl->library);
	}
}

void FontFace::setSize(float size)
{
	FT_Set_Char_Size(pimpl->face, int(size * 64), 0, 90, 0);
}

std::vector<int> FontFace::getCharCodes() const
{
	std::vector<int> result;
	FT_UInt index;
	for (FT_ULong charcode = FT_Get_First_Char(pimpl->face, &index); index != 0; charcode = FT_Get_Next_Char(pimpl->face, charcode, &index)) {
		result.push_back(charcode);
	}
	return result;
}

Vector2i FontFace::getGlyphSize(int charCode)
{
	int index = FT_Get_Char_Index(pimpl->face, charCode);
	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + String::integerToString(charCode));
	}
	auto metrics = pimpl->face->glyph->metrics;
	return Vector2i(metrics.width, metrics.height) / 64;
}

void FontFace::drawGlyph(Image& image, int charcode, Vector2i pos)
{
	auto glyph = pimpl->face->glyph;
	
	int index = FT_Get_Char_Index(pimpl->face, charcode);
	
	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + String::integerToString(charcode));
	}
	
	error = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
	if (error != 0) {
		throw Exception("Unable to render glyph " + String::integerToString(charcode));
	}
	
	auto bmp = glyph->bitmap;
	image.blitFrom(pos, reinterpret_cast<char*>(bmp.buffer), bmp.width, bmp.rows, bmp.pitch, 1);
}
