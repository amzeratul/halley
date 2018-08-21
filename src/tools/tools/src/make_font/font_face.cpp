#include "halley/tools/make_font/font_face.h"
#include <ft2build.h>
#include <halley/text/halleystring.h>
#include <memory>
#include <halley/support/exception.h>
#include <halley/maths/vector2.h>
#include <halley/file_formats/image.h>
#include FT_FREETYPE_H
#include "halley/text/string_converter.h"

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
	int error = FT_Init_FreeType(&pimpl->library);
	if (error) {
		throw Exception("Unable to initialize FreeType", HalleyExceptions::Graphics);
	}
	error = FT_New_Face(pimpl->library, filename.c_str(), 0, &pimpl->face);
	if (error) {
		throw Exception("Unable to load font face", HalleyExceptions::Graphics);
	}
}

FontFace::FontFace(gsl::span<const gsl::byte> data)
	: pimpl (std::make_unique<FontFacePimpl>())
{
	int error = FT_Init_FreeType(&pimpl->library);
	if (error) {
		throw Exception("Unable to initialize FreeType", HalleyExceptions::Graphics);
	}
	error = FT_New_Memory_Face(pimpl->library, reinterpret_cast<const FT_Byte*>(data.data()), FT_Long(data.size()), 0, &pimpl->face);
	if (error) {
		throw Exception("Unable to load font face", HalleyExceptions::Graphics);
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

String FontFace::getName() const
{
	return pimpl->face->family_name + String(" ") + pimpl->face->style_name;
}

float FontFace::getSize() const
{
	return size;
}

float FontFace::getHeight() const
{
	return pimpl->face->height * size / pimpl->face->units_per_EM;
}

float FontFace::getAscender() const
{
	return pimpl->face->ascender * size / pimpl->face->units_per_EM;
}

Vector<int> FontFace::getCharCodes() const
{
	Vector<int> result = {0};
	FT_UInt index;
	for (FT_ULong charcode = FT_Get_First_Char(pimpl->face, &index); index != 0; charcode = FT_Get_Next_Char(pimpl->face, charcode, &index)) {
		if (charcode != 0) {
			result.push_back(charcode);
		}
	}
	return result;
}

Vector2i FontFace::getGlyphSize(int charCode) const
{
	int index = charCode == 0 ? 0 : FT_Get_Char_Index(pimpl->face, charCode);
	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charCode), HalleyExceptions::Graphics);
	}
	auto metrics = pimpl->face->glyph->metrics;
	return Vector2i(metrics.width, metrics.height) / 64;
}

void FontFace::drawGlyph(Image& image, int charcode, Vector2i pos) const
{
	auto glyph = pimpl->face->glyph;
	
	int index = charcode == 0 ? 0 : FT_Get_Char_Index(pimpl->face, charcode);
	
	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charcode), HalleyExceptions::Graphics);
	}
	
	error = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
	if (error != 0) {
		throw Exception("Unable to render glyph " + toString(charcode), HalleyExceptions::Graphics);
	}
	
	auto bmp = glyph->bitmap;
	image.blitFrom(pos, reinterpret_cast<char*>(bmp.buffer), bmp.width, bmp.rows, bmp.pitch, 1);
}

FontMetrics FontFace::getMetrics(int charcode, float scale) const
{
	int index = FT_Get_Char_Index(pimpl->face, charcode);

	int error = FT_Load_Glyph(pimpl->face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charcode), HalleyExceptions::Graphics);
	}

	FontMetrics result;

	float multiplier = scale / 64.0f;
	auto& metrics = pimpl->face->glyph->metrics;
	result.advance = Vector2f(Vector2i(metrics.horiAdvance, metrics.vertAdvance)) * multiplier;
	result.bearingHorizontal = Vector2f(Vector2i(metrics.horiBearingX, metrics.horiBearingY)) * multiplier;
	result.bearingVertical = Vector2f(Vector2i(metrics.vertBearingX, metrics.vertBearingY)) * multiplier;
	
	return result;
}

Vector<KerningPair> FontFace::getKerning(const Vector<int>& codes) const
{
	Vector<KerningPair> results;
	if (!FT_HAS_KERNING(pimpl->face)) {
		return results;
	}
	
	Vector<int> indices;
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
