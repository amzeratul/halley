#pragma once
#include <type_traits>

namespace Halley
{
	class FontFacePimpl;

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
		String getName();
		float getSize();
		float getHeight();

		std::vector<int> getCharCodes() const;
		Vector2i getGlyphSize(int charCode);
		
		void drawGlyph(Image& image, int charcode, Vector2i pos);
		FontMetrics getMetrics(int charcode, float scale = 1.0f);

		std::vector<KerningPair> getKerning(const std::vector<int>& codes);

	private:
		std::unique_ptr<FontFacePimpl> pimpl;
		float size = 0;
	};
}
