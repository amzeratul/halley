#pragma once
#include <halley/maths/vector2.h>
#include <halley/text/halleystring.h>
#include <gsl/gsl>

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
		explicit FontFace(String filename);
		explicit FontFace(gsl::span<const gsl::byte> data);
		~FontFace();

		void setSize(float size);
		String getName() const;
		float getSize() const;
		float getHeight() const;
		float getAscender() const;

		Vector<int> getCharCodes() const;
		Vector2i getGlyphSize(int charCode) const;
		
		void drawGlyph(Image& image, int charcode, Vector2i pos) const;
		FontMetrics getMetrics(int charcode, float scale = 1.0f) const;

		Vector<KerningPair> getKerning(const Vector<int>& codes) const;

	private:
		std::unique_ptr<FontFacePimpl> pimpl;
		float size = 0;
	};
}
