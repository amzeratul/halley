#include "font_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/maths/vector2.h"
#include "halley/maths/range.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/file_formats/image.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/tools/file/filesystem.h"
#include "halley/graphics/text/font.h"

using namespace Halley;

void FontImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	const auto& meta = asset.inputFiles.at(0).metadata;
	const float radius = meta.getFloat("radius", 8);
	Vector2i imgSize;
	if (meta.hasKey("width") && meta.hasKey("height")) {
		imgSize.x = meta.getInt("width");
		imgSize.y = meta.getInt("height");
	}
	const float fontSize = meta.getFloat("fontSize", 0);
	const float replacementScale = meta.getFloat("replacementScale", 1.0f);

	Vector<Bytes> fontData;

	if (meta.hasKey("importFallbackFiles")) {
		for (auto& importFallbackFont: meta.getString("importFallbackFiles").split(',')) {
			auto path = asset.inputFiles[0].name.parentPath() / String::trimSpaces(importFallbackFont.trimBoth());
			auto bs = collector.readAdditionalFile(path);
			if (bs.empty()) {
				Logger::logError("Could not find importFallbackFile \"" + path.getString(false) + "\" for " + asset.assetId);
			} else {
				fontData += std::move(bs);
			}
		}
	}

	Vector<gsl::span<const std::byte>> fontDataSpans;
	fontDataSpans += asset.inputFiles[0].data.const_byte_span();
	for (auto& data: fontData) {
		fontDataSpans += data.const_byte_span();
	}

	FontGenerator gen(false, [&] (float progress, const String& label)
	{
		return collector.reportProgress(progress, label);
	});

	FontGenerator::FontSizeInfo sizeInfo;
	if (fontSize != 0) {
		sizeInfo.fontSize = fontSize;
	}
	if (imgSize != Vector2i()) {
		sizeInfo.imageSize = imgSize;
	}
	sizeInfo.replacementScale = replacementScale;

	auto result = gen.generateFont(meta, fontDataSpans.const_span(), sizeInfo, radius, getCharactersToImport(meta, collector));
	if (!result.success) {
		return;
	}

	auto fontName = result.font->getName();

	collector.output(fontName, AssetType::Font, Serializer::toBytes(*result.font));

	if (meta.hasKey("filtering")) {
		result.imageMeta->set("filtering", meta.getBool("filtering"));
	}
	result.imageMeta->set("compressAs", "png");

	ImportingAsset image;
	image.assetId = "fontTex/" + fontName;
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(fontName, Serializer::toBytes(*result.image), *result.imageMeta));
	collector.addAdditionalAsset(std::move(image));
}

Vector<char32_t> FontImporter::getCharactersToImport(const Metadata& meta, IAssetCollector& collector) const
{
	std::set<char32_t> chars;

	auto range = Range<int>(meta.getInt("rangeStart", 0), meta.getInt("rangeEnd", 255));
	for (int i = range.start; i <= range.end; ++i) {
		chars.insert(i);
	}

	for (int c: meta.getString("extraCharacters", "").getUTF32()) {
		chars.insert(c);
	}

	for (auto script: meta.getString("scripts", "").split(',')) {
		script.trimBoth();
		getCharactersForScript(chars, script.asciiLower(), collector);
	}

	Vector<char32_t> result;
	result.reserve(chars.size());
	for (auto& c: chars) {
		result.push_back(c);
	}
	return result;
}

void FontImporter::getCharactersForScript(std::set<char32_t>& charsOutput, const String& script, IAssetCollector& collector) const
{
	// See: https://en.wikipedia.org/wiki/Plane_(Unicode)#Basic_Multilingual_Plane

	if (script == "latin") {
		addCharactersFromFile(charsOutput, "latin", collector);
	}
	if (script == "latin-extended-a") {
		addCharactersFromFile(charsOutput, "latin_extended_a", collector);
	}
	if (script == "latin-extended-b") {
		addCharactersFromFile(charsOutput, "latin_extended_b", collector);
	}
	if (script == "greek") {
		addCharactersFromFile(charsOutput, "greek", collector);
	}
	if (script == "cyrillic") {
		addCharactersFromFile(charsOutput, "cyrillic", collector);
	}
	if (script == "hebrew") {
		addCharactersFromFile(charsOutput, "hebrew", collector);
	}
	if (script == "arabic") {
		addCharactersFromFile(charsOutput, "arabic", collector);
	}
	if (script == "hiragana") {
		addCharactersFromFile(charsOutput, "hiragana", collector);
	}
	if (script == "katakana") {
		addCharactersFromFile(charsOutput, "katakana", collector);
	}
	if (script == "japanese") {
		addCharactersFromFile(charsOutput, "japanese", collector);
	}
	if (script == "chinese-simplified") {
		addCharactersFromFile(charsOutput, "chinese_simplified", collector);
	}
	if (script == "chinese-traditional") {
		addCharactersFromFile(charsOutput, "chinese_traditional", collector);
	}
	if (script == "korean" || script == "hangul") {
		addCharactersFromFile(charsOutput, "korean", collector);
	}
}

void FontImporter::addCharactersFromFile(std::set<char32_t>& output, const String& filename, IAssetCollector& collector) const
{
	const auto bytes = collector.readAdditionalFile("font_charset/" + filename + ".yaml");
	if (bytes.empty()) {
		Logger::logError("Could not find character set file: " + filename);
		return;
	}

	const auto config = YAMLConvert::parseConfig(bytes);
	for (const auto& entry: config.getRoot().asSequence()) {
		if (entry.hasKey("file")) {
			addCharactersFromFile(output, entry["file"].asString(), collector);
		}
		if (entry.hasKey("range")) {
			auto range = entry["range"].asIntRange();
			addRange(output, range.start, range.end);
		}
		if (entry.hasKey("list")) {
			auto list = entry["list"].asString().getUTF32();
			addList(output, list);
		}
	}
}

void FontImporter::addRange(std::set<char32_t>& output, char32_t codePointStart, char32_t codePointEnd) const
{
	for (char32_t i = codePointStart; i <= codePointEnd; ++i) {
		output.insert(i);
	}
}

void FontImporter::addList(std::set<char32_t>& output, gsl::span<const char32_t> chars) const
{
	for (const auto& c: chars) {
		output.insert(c);
	}
}
