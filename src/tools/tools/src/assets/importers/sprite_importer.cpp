#include "sprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/sprite/animation.h"
#include "halley/data_structures/bin_pack.h"
#include "halley/text/string_converter.h"
#include "../../sprites/aseprite_reader.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/support/logger.h"
#include "halley/utils/hash.h"

using namespace Halley;


void SpriteImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	String baseSpriteSheetName = Path(asset.assetId).replaceExtension("").string();
	std::map<String, Vector<ImageData>> totalGroupedFrames;

	std::optional<Metadata> startMeta;

	std::optional<String> palette;
	bool powerOfTwo = true;

	for (auto& inputFile: asset.inputFiles) {
		auto fileInputId = Path(inputFile.name).dropFront(1);
		const String baseSpriteName = fileInputId.replaceExtension("").string();

		// Meta
		Metadata meta = inputFile.metadata;
		if (!startMeta) {
			startMeta = meta;
		}
		Vector2i pivot;
		pivot.x = meta.getInt("pivotX", 0);
		pivot.y = meta.getInt("pivotY", 0);
		Vector4s slices;
		slices.x = gsl::narrow<short, int>(meta.getInt("slice_left", 0));
		slices.y = gsl::narrow<short, int>(meta.getInt("slice_top", 0));
		slices.z = gsl::narrow<short, int>(meta.getInt("slice_right", 0));
		slices.w = gsl::narrow<short, int>(meta.getInt("slice_bottom", 0));
		const bool trim = meta.getBool("trim", true);
		const int padding = meta.getInt("padding", 0);

		// Palette
		auto thisPalette = meta.getString("palette", "");
		if (palette) {
			if (thisPalette != palette.value()) {
				throw Exception("Incompatible palettes in atlas \"" + asset.assetId + "\". Previously using \"" + palette.value() + "\", now trying to use \"" + thisPalette + "\"", HalleyExceptions::Tools);
			}
		} else {
			palette = thisPalette;
		}

		// Power of two
		if (asset.inputFiles.size() == 1) {
			powerOfTwo = meta.getBool("powerOfTwo", true);
		}

		// Import image data
		std::map<String, Vector<ImageData>> groupedFrames;
		
		if (inputFile.name.getExtension() == ".ase" || inputFile.name.getExtension() == ".aseprite") {
			// Import Aseprite file
			auto groupSeparated = meta.getBool("group_separated", false);
			auto sequenceSeparated = meta.getBool("sequence_separated", false);
			groupedFrames = AsepriteReader::importAseprite(baseSpriteName, inputFile.name, gsl::as_bytes(gsl::span<const Byte>(inputFile.data)), trim, padding, groupSeparated, sequenceSeparated);
		} else {
			// Bitmap
			auto span = gsl::as_bytes(gsl::span<const Byte>(inputFile.data));
			auto image = std::make_unique<Image>(span, fromString<Image::Format>(meta.getString("format", "undefined")));
			Rect4i clip = trim ? image->getTrimRect() : image->getRect(); // Be careful, make sure this is done before the std::move() below
			if (!clip.isEmpty()) { // Padding an empty sprite can have all kinds of unexpected effects, and also affect performance
				clip = clip.grow(padding);
			}
			
			groupedFrames[""] = Vector<ImageData>();
			auto& frames = groupedFrames[""];
			auto& imgData = frames.emplace_back();
			imgData.clip = clip;
			imgData.img = std::move(image);
			imgData.duration = 100;
			imgData.filenames.emplace_back(":img:" + fileInputId.toString());
			imgData.origFilename = inputFile.name.toString();
			imgData.frameNumber = 0;
			imgData.origFrameNumber = 0;
			imgData.sequenceName = "";
		}

		auto oneAtlas = meta.getString("atlas", "");
		
		for (auto& frames : groupedFrames) {
			// Update frames with pivot and slices
			for (auto& f : frames.second) {
				f.pivot = pivot;
				f.slices = slices;
			}

			// Split into a grid
			const Vector2i grid(meta.getInt("tileWidth", 0), meta.getInt("tileHeight", 0));
			if (grid.x > 0 && grid.y > 0) {
				frames.second = splitImagesInGrid(frames.second, grid);
			}
			
			auto spriteSheetName = baseSpriteSheetName;// +(frames.first.isEmpty() ? "" : ":" + frames.first);
			auto spriteName = baseSpriteName + (frames.first.isEmpty() ? "" : ":" + frames.first);
			Animation animation = generateAnimation(spriteName, spriteSheetName, meta, frames.second);
			collector.output(spriteName, AssetType::Animation, Serializer::toBytes(animation), {}, "pc", inputFile.name);

			Vector<ImageData> totalFrames;
			std::move(frames.second.begin(), frames.second.end(), std::back_inserter(totalFrames));
			totalGroupedFrames[spriteSheetName + " " + spriteName] = std::move(totalFrames);
		}
		
	}

	// Generate atlas + spritesheet
	Vector<ImageData> totalFrames;
	for (auto& frames : totalGroupedFrames) {
		std::move(frames.second.begin(), frames.second.end(), std::back_inserter(totalFrames));
	}

	// Metafile
	Metadata meta;
	if (startMeta) {
		meta = startMeta.value();
	}

	// Create the atlas
	auto groupAtlasName = asset.assetId;
	auto spriteSheet = std::make_shared<SpriteSheet>();
	ConfigNode spriteInfo;
	auto atlasImage = spriteSheet->generateAtlas(totalFrames, spriteInfo, powerOfTwo);
	spriteSheet->setTextureName(groupAtlasName);
	spriteSheet->setDefaultMaterialName(meta.getString("material", meta.getString("defaultMaterial", MaterialDefinition::defaultMaterial)));

	// Metafile parameters
	auto size = atlasImage->getSize();
	if (palette) {
		meta.set("palette", palette.value());
	}
	if (!powerOfTwo) {
		meta.set("powerOfTwo", false);
	}
	meta.set("width", size.x);
	meta.set("height", size.y);
	meta.set("compression", "raw_image");

	// Write atlas image
	ImportingAsset image;
	image.assetId = groupAtlasName;
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(groupAtlasName, Serializer::toBytes(*atlasImage), meta));
	image.options.ensureType(ConfigNodeType::Map);
	image.options["sprites"] = std::move(spriteInfo);
	collector.addAdditionalAsset(std::move(image));

	// Write spritesheet
	spriteSheet->setAssetId(baseSpriteSheetName);
	collector.output(baseSpriteSheetName, AssetType::SpriteSheet, Serializer::toBytes(*spriteSheet, SerializerOptions(SerializerOptions::maxVersion)));

	// Write sprites
	std::map<String, String> primaryPaths;
	for (const auto& frame: totalFrames) {
		for (const auto& filename: frame.filenames) {
			if (filename.startsWith(":img:")) {
				primaryPaths[filename.mid(5)] = frame.origFilename;
			}
		}
	}
	for (const auto& [k, idx]: spriteSheet->getSpriteNameMap()) {
		if (k.startsWith(":img:")) {
			const auto iter = primaryPaths.find(k.mid(5));
			SpriteResource spriteRes(spriteSheet, idx);
			collector.output(k.mid(5), AssetType::Sprite, Serializer::toBytes(spriteRes, SerializerOptions(SerializerOptions::maxVersion)), {}, "pc", iter->second);
		}
	}
}

String SpriteImporter::getAssetId(const Path& file, const std::optional<Metadata>& metadata) const
{
	if (metadata) {
		String atlas = metadata->getString("atlas", "");
		if (atlas != "") {
			return atlas;
		}
	}
	return IAssetImporter::getAssetId(file, metadata);
}

Animation SpriteImporter::generateAnimation(const String& spriteName, const String& spriteSheetName, const Metadata& meta, const Vector<ImageData>& frameData)
{
	Animation animation;

	animation.setName(spriteName);
	animation.setMaterialName(meta.getString("material", MaterialDefinition::defaultMaterial));
	animation.setSpriteSheetName(spriteSheetName);

	// Generate directions
	int numExplicitDirs = 0;
	{
		std::set<String> directionNames;
		for (const auto& frame: frameData) {
			if (!frame.direction.isEmpty()) {
				if (directionNames.find(frame.direction) == directionNames.end()) {
					directionNames.insert(frame.direction);
					numExplicitDirs++;
				}
			}
		}

		if (numExplicitDirs == 0) {
			directionNames.insert("right");
		}

		for (const auto& dir: directionNames) {
			animation.addDirection(AnimationDirection(dir, dir, false));
		}

		auto hasAnim = [&](const String& name)
		{
			return directionNames.find(name) != directionNames.end();
		};
		auto replaceAnim = [&](const String& toAdd, const String& base)
		{
			if (!hasAnim(toAdd) && hasAnim(base)) {
				animation.addDirection(AnimationDirection(toAdd, base, true), animation.getDirection(base).getId());
			}
		};

		replaceAnim("left", "right");
		replaceAnim("up_left", "up_right");
		replaceAnim("down_left", "down_right");
	}
		
	std::map<String, AnimationSequence> sequences;
	std::map<String, std::set<String>> directionsPerSequence;

	const auto fallbackSequenceName = meta.getString("fallbackSequence", "");
	for (const auto& frame : frameData) {
		String sequence = frame.sequenceName;

		const auto i = sequences.find(sequence);
		if (i == sequences.end()) {
			sequences[sequence] = AnimationSequence(sequence, true, false, sequence == fallbackSequenceName);
		}

		directionsPerSequence[sequence].insert(frame.direction);
	}

	for (const auto& frame : frameData) {
		String sequence = frame.sequenceName;
		auto& seq = sequences[sequence];
		if (static_cast<int>(seq.numFrameDefinitions()) == frame.frameNumber) {
			auto filename = frame.filenames.at(0);
			if (!directionsPerSequence.at(sequence).empty() && !frame.direction.isEmpty()) {
				filename = filename.replaceAll("_" + frame.direction, "_%dir%");
			}
			seq.addFrame(AnimationFrameDefinition(frame.frameNumber, frame.duration, filename));
		}
	}

	for (auto& seq: sequences) {
		animation.addSequence(seq.second);
	}

	animation.addActionPoints(meta.getValue("actionPoints"));

	return animation;
}

Vector<SpriteImporter::ImageData> SpriteImporter::splitImagesInGrid(const Vector<ImageData>& images, Vector2i grid)
{
	Vector<ImageData> result;

	for (const auto& src: images) {
		auto imgSize = src.img->getSize();
		int nX = imgSize.x / grid.x;
		int nY = imgSize.y / grid.y;

		for (int y = 0; y < nY; ++y) {
			for (int x = 0; x < nX; ++x) {
				auto img = std::make_unique<Image>(Image::Format::RGBA, grid);
				img->blitFrom(Vector2i(), *src.img, Rect4i(Vector2i(x, y) * grid, grid.x, grid.y));
				Rect4i trimRect = img->getTrimRect();
				if (trimRect.getWidth() > 0 && trimRect.getHeight() > 0) {
					result.emplace_back();
					auto& dst = result.back();

					String suffix = "_" + toString(x) + "_" + toString(y);

					dst.duration = src.duration;
					dst.frameNumber = src.frameNumber;
					dst.origFrameNumber = src.origFrameNumber;
					dst.filenames.emplace_back(src.filenames.at(0) + suffix);
					dst.sequenceName = src.sequenceName + suffix;
					dst.img = std::move(img);
					dst.clip = Rect4i({}, grid);
				}
			}
		}
	}

	return result;
}
