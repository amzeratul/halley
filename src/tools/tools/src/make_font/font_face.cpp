#include "halley/tools/make_font/font_face.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <halley/text/halleystring.h>
#include <memory>
#include <halley/support/exception.h>
#include <halley/maths/vector2.h>
#include <halley/file_formats/image.h>
#include "halley/text/string_converter.h"

using namespace Halley;

FontFace::FontFace(gsl::span<const std::byte> data)
{
	int error = FT_Init_FreeType(&library);
	if (error) {
		throw Exception("Unable to initialize FreeType", HalleyExceptions::Graphics);
	}

	faces.resize(1);
	error = FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(data.data()), FT_Long(data.size()), 0, &faces[0]);
	if (error) {
		throw Exception("Unable to load font face", HalleyExceptions::Graphics);
	}
}

FontFace::FontFace(gsl::span<const gsl::span<const std::byte>> datas)
{
	int error = FT_Init_FreeType(&library);
	if (error) {
		throw Exception("Unable to initialize FreeType", HalleyExceptions::Graphics);
	}

	for (auto& data: datas) {
		faces.emplace_back();
		error = FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(data.data()), FT_Long(data.size()), 0, &faces.back());
		if (error) {
			throw Exception("Unable to load font face", HalleyExceptions::Graphics);
		}
	}
}

FontFace::~FontFace()
{
	for (auto& face: faces) {
		FT_Done_Face(face);
	}
	if (library) {
		FT_Done_FreeType(library);
	}
}

void FontFace::setSize(float sz)
{
	size = sz;
	for (auto& face: faces) {
		FT_Set_Char_Size(face, 0, lround(size * 64), 72, 0);
	}
}

String FontFace::getName() const
{
	return faces[0]->family_name + String(" ") + faces[0]->style_name;
}

float FontFace::getSize() const
{
	return size;
}

float FontFace::getHeight() const
{
	auto face = faces[0];
	if (face->height > 0 && face->units_per_EM > 0) {
		return face->height * size / face->units_per_EM;
	} else {
		return face->size->metrics.height / 64.0f;
	}
}

float FontFace::getAscender() const
{
	auto face = faces[0];
	if (face->units_per_EM > 0) {
		return face->ascender * size / face->units_per_EM;
	} else {
		return face->size->metrics.ascender / 64.0f;
	}
}

HashSet<int> FontFace::getCharCodes() const
{
	HashSet<int> result;
	result.insert(0);

	FT_UInt index;
	for (auto& face: faces) {
		for (FT_ULong charcode = FT_Get_First_Char(face, &index); index != 0; charcode = FT_Get_Next_Char(face, charcode, &index)) {
			if (charcode != 0) {
				result.insert(charcode);
			}
		}
	}
	return result;
}

Vector2i FontFace::getGlyphSize(int charCode) const
{
	auto face = getFreeTypeFace(charCode);
	int index = charCode == 0 ? 0 : FT_Get_Char_Index(face, charCode);
	int error = FT_Load_Glyph(face, index, FT_LOAD_NO_HINTING);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charCode), HalleyExceptions::Graphics);
	}
	auto metrics = face->glyph->metrics;
	return Vector2i(metrics.width, metrics.height) / 64;
}

void FontFace::drawGlyph(Image& image, int charcode, Vector2i pos) const
{
	auto face = getFreeTypeFace(charcode);
	auto glyph = face->glyph;
	
	int index = charcode == 0 ? 0 : FT_Get_Char_Index(face, charcode);
	
	int error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charcode), HalleyExceptions::Graphics);
	}
	
	error = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
	if (error != 0) {
		throw Exception("Unable to render glyph " + toString(charcode), HalleyExceptions::Graphics);
	}
	
	auto bmp = glyph->bitmap;
	image.blitFrom(pos, gsl::span<unsigned char>(bmp.buffer, bmp.rows * bmp.pitch), bmp.width, bmp.rows, bmp.pitch, 1);
}

FontMetrics FontFace::getMetrics(int charcode, float scale) const
{
	auto face = getFreeTypeFace(charcode);
	int index = FT_Get_Char_Index(face, charcode);

	int error = FT_Load_Glyph(face, index, FT_LOAD_NO_HINTING);
	if (error) {
		throw Exception("Unable to load glyph " + toString(charcode), HalleyExceptions::Graphics);
	}

	FontMetrics result;

	float multiplier = scale / 64.0f;
	auto& metrics = face->glyph->metrics;
	result.advance = Vector2f(Vector2i(metrics.horiAdvance, metrics.vertAdvance)) * multiplier;
	result.bearingHorizontal = Vector2f(Vector2i(metrics.horiBearingX, metrics.horiBearingY)) * multiplier;
	result.bearingVertical = Vector2f(Vector2i(metrics.vertBearingX, metrics.vertBearingY)) * multiplier;
	
	return result;
}

Vector<KerningPair> FontFace::getKerning(const Vector<int>& codes) const
{
	Vector<KerningPair> results;

	if (std::none_of(faces.begin(), faces.end(), [&] (auto& f) { return FT_HAS_KERNING(f); })) {
		return results;
	}
	
	HashMap<int32_t, std::pair<int, FT_Face>> indices;
	for (int code: codes) {
		if (code != 0) {
			auto face = getFreeTypeFace(code);
			indices[code] = { FT_Get_Char_Index(face, code), face };
		}
	}

	for (int left: codes) {
		if (left == 0) {
			continue;
		}
		const auto indexLeft = indices.at(left);

		for (int right: codes) {
			if (right == 0) {
				continue;
			}
			const auto indexRight = indices.at(right);

			if (indexLeft.second == indexRight.second) {
				FT_Vector result;
				FT_Get_Kerning(indexLeft.second, indexLeft.first, indexRight.first, FT_KERNING_UNFITTED, &result);
				if (result.x != 0 || result.y != 0) {
					const auto kerning = Vector2f(result.x / 64.0f, result.y / 64.0f);
					results.emplace_back(KerningPair(left, right, kerning));
				}
			}
		}
	}

	return results;
}

FT_Library FontFace::getFreeTypeLib() const
{
	return library;
}

FT_Face FontFace::getFreeTypeFace(std::optional<int> character) const
{
	if (faces.empty()) {
		return nullptr;
	}

	if (character && faces.size() > 1) {
		for (auto& face: faces) {
			if (FT_Get_Char_Index(face, *character) != 0) {
				return face;
			}
		}
	}

	return faces[0];
}
