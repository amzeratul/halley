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

void FontFace::setSize(float sz)
{
	size = sz;
	FT_Set_Char_Size(pimpl->face, int(size * 64), 0, 72, 0);
}

String FontFace::getName()
{
	return pimpl->face->family_name + String(" ") + pimpl->face->style_name;
}

float FontFace::getSize()
{
	return size;
}

float FontFace::getHeight()
{
	return pimpl->face->height * size / pimpl->face->units_per_EM;
}

float FontFace::getAscender()
{
	return pimpl->face->ascender * size / pimpl->face->units_per_EM;
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
	int index = charCode == 0 ? 0 : FT_Get_Char_Index(pimpl->face, charCode);
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
	
	int index = charcode == 0 ? 0 : FT_Get_Char_Index(pimpl->face, charcode);
	
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

FontMetrics FontFace::getMetrics(int charcode, float scale)
{
	int index = FT_Get_Char_Index(pimpl->face, charcode);

	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + String::integerToString(charcode));
	}

	FontMetrics result;

	float multiplier = scale / 64.0f;
	auto& metrics = pimpl->face->glyph->metrics;
	result.advance = Vector2f(Vector2i(metrics.horiAdvance, metrics.vertAdvance)) * multiplier;
	result.bearingHorizontal = Vector2f(Vector2i(metrics.horiBearingX, metrics.horiBearingY)) * multiplier;
	result.bearingVertical = Vector2f(Vector2i(metrics.vertBearingX, metrics.vertBearingY)) * multiplier;
	
	return result;
}

std::vector<KerningPair> FontFace::getKerning(const std::vector<int>& codes)
{
	std::vector<KerningPair> results;
	if (!FT_HAS_KERNING(pimpl->face)) {
		return results;
	}
	
	std::vector<int> indices;
	for (int code: codes) {
		int index = FT_Get_Char_Index(pimpl->face, code);
		if (code != 0) {
			indices.push_back(index);
		}
	}
	
	for (int left: indices) {
		for (int right: indices) {
			FT_Vector result;
			FT_Get_Kerning(pimpl->face, left, right, FT_KERNING_UNFITTED, &result);
			if (result.x != 0 || result.y != 0) {
				results.emplace_back(KerningPair(left, right, Vector2f(result.x / 64.0f, result.y / 64.0f)));
			}
		}
	}

	return results;
}
