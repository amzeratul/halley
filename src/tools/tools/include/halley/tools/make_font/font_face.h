#pragma once
#include <halley/maths/vector2.h>
#include <halley/text/halleystring.h>
#include <gsl/gsl>

struct FT_LibraryRec_;
typedef FT_LibraryRec_ *FT_Library;
struct FT_FaceRec_;
typedef FT_FaceRec_ *FT_Face;

namespace Halley
{
	class FontFacePimpl;
	class Image;

	struct FontMetrics
	{
		Vector2f advance;
		Vector2f bearingHorizontal;
		Vector2f bearingVertical;
	};

	struct KerningPair
	{
		int left;
		int right;
		Vector2f kerning;

		KerningPair(int left, int right, Vector2f kerning) : left(left), right(right), kerning(kerning) {}
	};

	class FontFace
	{
	public:
		explicit FontFace(gsl::span<const std::byte> data);
		explicit FontFace(gsl::span<const gsl::span<const std::byte>> datas);
		~FontFace();

		void setSize(float size);
		String getName() const;
		float getSize() const;
		float getHeight() const;
		float getAscender() const;

		HashSet<int> getCharCodes() const;
		Vector2i getGlyphSize(int charCode) const;
		
		void drawGlyph(Image& image, int charcode, Vector2i pos) const;
		FontMetrics getMetrics(int charcode, float scale = 1.0f) const;

		Vector<KerningPair> getKerning(const Vector<int>& codes) const;

		FT_Library getFreeTypeLib() const;
		FT_Face getFreeTypeFace(std::optional<int> character) const;

	private:
		FT_Library library = nullptr;
		Vector<FT_Face> faces;
		float size = 0;
	};
}
