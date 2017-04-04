#include "bitmap_font_importer.h"
#include "halley/support/exception.h"
#include "../contrib/tinyxml/ticpp.h"
#include "halley/core/graphics/text/font.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/file_formats/image.h"

using namespace Halley;

String BitmapFontImporter::getAssetId(const Path& file) const
{
	return file.getStem().getString();
}

void BitmapFontImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Bytes xmlData;
	Path xmlPath;
	Bytes pngData;
	Path pngPath;

	for (auto& file: asset.inputFiles) {
		if (file.name.getExtension() == ".xml") {
			xmlPath = Path("font") / file.name.getFilename();
			xmlData = file.data;
		} else if (file.name.getExtension() == ".png") {
			pngPath = Path("font") / file.name.getFilename();
			pngData = file.data;
		}
	}

	if (xmlData.empty()) {
		throw Exception("Missing XML data for bitmap font.");
	}
	if (pngData.empty()) {
		throw Exception("Missing PNG data for bitmap font.");
	}

	// Parse image
	Vector2i imageSize = Image::getImageSize(gsl::as_bytes(gsl::span<Byte>(pngData)));

	// Generate font from XML
	Font font = parseBitmapFontXML(imageSize, xmlData);
	collector.output(font.getName(), AssetType::Font, Serializer::toBytes(font));

	// Pass image forward
	ImportingAsset image;
	image.assetId = font.getName();
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(font.getName(), std::move(pngData)));
	collector.addAdditionalAsset(std::move(image));
}

Font BitmapFontImporter::parseBitmapFontXML(Vector2i imageSize, const Bytes& data)
{
	ticpp::Document doc;
	doc.Parse(String(reinterpret_cast<const char*>(data.data()), data.size()).cppStr());

	Vector2f imageScale = Vector2f(1.0f / imageSize.x, 1.0f / imageSize.y);

	for (auto fontIter = doc.FirstChild(); fontIter; fontIter = fontIter->NextSibling(false)) {
		if (fontIter->Value() == "Font") {
			String family;
			int fontHeight = 0;
			int fontSize = 0;

			auto fontElem = fontIter->ToElement();
			family = fontElem->GetAttribute("family");
			fontElem->GetAttribute("height", &fontHeight);
			fontElem->GetAttribute("size", &fontSize);

			Font font(family, family, 0, float(fontHeight), float(fontSize));

			ticpp::Iterator<ticpp::Element> child("Char");
			for (child = child.begin(fontIter); child != child.end(); ++child) {
				int charcode;
				Rect4f area;
				Vector2f size;
				Vector2f bearing;
				Vector2f advance;
				
				int width;
				child->GetAttribute("width", &width);
				advance.x = float(width);
				advance.y = 0;

				auto offsets = String(child->GetAttribute("offset")).split(' ');
				bearing.x = offsets[0].toFloat();
				bearing.y = -offsets[1].toFloat(); // ??

				Vector2f imgPos;
				auto rect = String(child->GetAttribute("rect")).split(' ');
				imgPos.x = rect[0].toFloat();
				imgPos.y = rect[1].toFloat();
				size.x = rect[2].toFloat();
				size.y = rect[3].toFloat();
				area = Rect4f(imgPos * imageScale, (imgPos + size) * imageScale);

				String code = child->GetAttribute("code");
				charcode = code.getUTF32()[0];

				font.addGlyph(Font::Glyph(charcode, area, size, bearing, bearing, advance));
			}

			return font;
		}
	}

	throw Exception("Invalid bitmap font format");
}
