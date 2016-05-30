#pragma once
#include <type_traits>
#include <halley/maths/vector2d.h>
#include <halley/text/halleystring.h>

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
		~FontFace();

		void setSize(float size);
		String getName() const;
		float getSize() const;
		float getHeight() const;
		float getAscender() const;

		std::vector<int> getCharCodes() const;
		Vector2i getGlyphSize(int charCode) const;
		
		void drawGlyph(Image& image, int charcode, Vector2i pos) const;
		FontMetrics getMetrics(int charcode, float scale = 1.0f) const;

		std::vector<KerningPair> getKerning(const std::vector<int>& codes) const;

	private:
		std::unique_ptr<FontFacePimpl> pimpl;
		float size = 0;
	};
}
