#include "audio_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/audio/vorbis_dec.h"
#include "halley/audio/resampler.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

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

	if (mainFile.getExtension() == ".ogg") { // assuming Ogg Vorbis
		// Load vorbis data
		VorbisData vorbis(resData);
		numChannels = vorbis.getNumChannels();
		sampleRate = vorbis.getSampleRate();

		// Decode
		if (sampleRate != 48000) {
			vorbis.getData(samples);
			needsResampling = true;
		}
	} else {
		throw Exception("Unsupported audio format: " + mainFile.getExtension());
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

static void onVorbisError(int error)
{
	String str;
	switch (error) {
	case OV_EREAD: str = "A read from media returned an error."; break;
	case OV_ENOTVORBIS: str = "Bitstream does not contain any Vorbis data."; break;
	case OV_EVERSION: str = "Vorbis version mismatch."; break;
	case OV_EBADHEADER: str = "Invalid Vorbis bitstream header."; break;
	case OV_EFAULT: str = "Internal logic fault; indicates a bug or heap/stack corruption."; break;
	case OV_HOLE: str = "Indicates there was an interruption in the data."; break;
	case OV_EBADLINK: str = "Indicates that an invalid stream section was supplied to libvorbisfile, or the requested link is corrupt."; break;
	case OV_EINVAL: str = "Indicates the initial file headers couldn't be read or are corrupt, or that the initial open call for vf failed."; break;
	case OV_ENOSEEK: str = "Stream is not seekable."; break;
	default: str = "Unknown error.";
	}
	throw Exception("Error opening Ogg Vorbis: "+str);
}

static void outputPacket(Bytes& dst, ogg_packet& packet)
{
	size_t size = dst.size();
	dst.resize(size + packet.bytes);
	memcpy(dst.data() + size, packet.packet, size);
}

Bytes AudioImporter::encodeVorbis(int nChannels, int sampleRate, gsl::span<const short> src)
{
	Bytes result;
	int ret = 0;

	// Based on steps from https://xiph.org/vorbis/doc/libvorbis/overview.html
	// 1.
	vorbis_info vi;
	vorbis_info_init(&vi);
	ret = vorbis_encode_init_vbr(&vi, long(nChannels), long(sampleRate), 0.5f);
	if (ret) {
		onVorbisError(ret);
	}

	// 2.
	vorbis_dsp_state v;
	vorbis_analysis_init(&v, &vi);

	// 3.
	ogg_packet op;
	ogg_packet op_comm;
	ogg_packet op_code;
	vorbis_comment vc;
	vorbis_comment_init(&vc);
	ret = vorbis_analysis_headerout(&v, &vc, &op, &op_comm, &op_code);
	if (ret) {
		onVorbisError(ret);
	}
	outputPacket(result, op);
	outputPacket(result, op_comm);
	outputPacket(result, op_code);

	// 4.
	vorbis_block vb;
	ret = vorbis_block_init(&v, &vb);
	if (!ret) {
		onVorbisError(ret);
	}

	// 5. / 6.
	float scale = 1.0f / 32768.0f;
	while (true) {
		// 5.1.
		size_t samplesToWrite = std::min(size_t(src.size() / nChannels), size_t(1024));
		float** buffers = vorbis_analysis_buffer(&v, samplesToWrite);
		for (size_t i = 0; i < nChannels; ++i) {
			for (size_t j = 0; j < samplesToWrite; ++j) {
				buffers[i][j] = src[j * nChannels + i] * scale;
			}
		}
		vorbis_analysis_wrote(&v, samplesToWrite);

		// 5.2.
		bool hasMoreBlocks = true;
		while (hasMoreBlocks) {
			ret = vorbis_analysis_blockout(&v, &vb);
			if (ret < 0) {
				onVorbisError(ret);
			}
			hasMoreBlocks = ret == 1;

			// 5.2.1.
			ret = vorbis_analysis(&vb, nullptr);
			if (!ret) {
				onVorbisError(ret);
			}
			
			// 5.2.2.
			ret = vorbis_bitrate_addblock(&vb);
			if (!ret) {
				onVorbisError(ret);
			}

			bool hasMorePackets = true;
			while (hasMorePackets) {
				ret = vorbis_bitrate_flushpacket(&v, &op);
				if (!ret) {
					onVorbisError(ret);
				}
				hasMorePackets = ret == 1;

				// 5.2.3.
				outputPacket(result, op);
			}
		}

		// We allow it to loop one more time after size is zero
		if (src.size() == 0) {
			break;
		}
		src = src.subspan(samplesToWrite * nChannels);
	}

	// 7.
	vorbis_block_clear(&vb);
	vorbis_comment_clear(&vc);
	vorbis_dsp_clear(&v);
	vorbis_info_clear(&vi);

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
