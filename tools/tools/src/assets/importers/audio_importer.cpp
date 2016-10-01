#include "audio_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/audio/vorbis_dec.h"
#include "halley/audio/resampler.h"

using namespace Halley;

std::vector<Path> AudioImporter::import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector)
{
	Path mainFile = asset.inputFiles.at(0).name;
	auto& rawData = asset.inputFiles[0].data;
	auto resData = std::make_shared<ResourceDataStatic>(rawData.data(), rawData.size(), mainFile.string(), false);
	Bytes encodedData;
	gsl::span<const gsl::byte> fileData(gsl::as_bytes(gsl::span<const Halley::Byte>(rawData)));

	std::vector<short> samples;
	int numChannels = 0;
	int sampleRate = 0;
	bool needsEncoding = false;
	bool needsResampling = false;

	if (true) { // is vorbis
		// Load vorbis data
		VorbisData vorbis(resData);
		numChannels = vorbis.getNumChannels();
		sampleRate = vorbis.getSampleRate();

		// Decode
		if (sampleRate != 48000) {
			vorbis.getData(samples);
			needsResampling = true;
		}
	}

	// Resample
	if (needsResampling) {
		// Resample
		auto dst = resample(numChannels, sampleRate, 48000, samples);
		samples = std::move(dst);
		dst.clear();
		sampleRate = 48000;
		needsEncoding = true;
	}

	// Encode to vorbis
	if (needsEncoding) {
		encodedData = encodeVorbis(numChannels, sampleRate, gsl::span<short>(samples));
		fileData = gsl::as_bytes(gsl::span<const Halley::Byte>(encodedData));
		samples.clear();
	}

	// Simply save file
	FileSystem::writeFile(dstDir / mainFile, fileData);

	// Write metafile
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}
	meta.set("channels", numChannels);
	meta.set("sampleRate", sampleRate);
	Path metaPath = mainFile.replaceExtension(mainFile.getExtension() + ".meta");
	FileSystem::writeFile(dstDir / metaPath, Serializer::toBytes(meta));

	return { mainFile, metaPath };
}

Bytes AudioImporter::encodeVorbis(int nChannels, int sampleRate, gsl::span<const short> src)
{
	Bytes result;

	// TODO

	return result;
}

std::vector<short> AudioImporter::resample(int numChannels, int from, int to, gsl::span<const short> src)
{
	AudioResampler resampler(from, to, numChannels, 1.0f);
	std::vector<short> dst(resampler.numOutputSamples(src.size()) + 1024);
	auto result = resampler.resampleInterleaved(src, dst);
	if (result.nRead * numChannels != src.size()) {
		throw Exception("Only read " + toString(result.nRead) + " samples, expected " + toString(src.size()));
	}
	if (result.nWritten * numChannels == dst.size()) {
		throw Exception("Resample dst buffer overflow.");
	}
	dst.resize(result.nWritten);
	return dst;
}
