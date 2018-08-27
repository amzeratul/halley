#include "audio_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/audio/vorbis_dec.h"
#include "halley/audio/resampler.h"

#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"
#include "halley/concurrency/concurrent.h"
#include "halley/tools/file/filesystem.h"
#include "halley/text/string_converter.h"
#include "halley/resources/resource_data.h"
#include "halley/support/logger.h"

using namespace Halley;

void AudioImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path mainFile = asset.inputFiles.at(0).name;
	auto& rawData = asset.inputFiles[0].data;
	auto resData = std::make_shared<ResourceDataStatic>(rawData.data(), rawData.size(), mainFile.string(), false);
	Bytes encodedData;
	const Bytes* fileData = &rawData;

	std::vector<std::vector<float>> samples;
	int numChannels = 0;
	int sampleRate = 0;
	bool needsEncoding = false;
	bool needsResampling = false;

	if (mainFile.getExtension() == ".ogg") { // assuming Ogg Vorbis
		// Load vorbis data
		VorbisData vorbis(resData);
		numChannels = vorbis.getNumChannels();
		size_t numSamples = vorbis.getNumSamples();
		sampleRate = vorbis.getSampleRate();
		samples.resize(numChannels);
		for (size_t i = 0; i < numChannels; ++i) {
			samples[i].resize(numSamples);
		}

		// Decode
		if (sampleRate != 48000) {
			vorbis.read(samples);
			needsResampling = true;
		}
	} else {
		throw Exception("Unsupported audio format: " + mainFile.getExtension(), HalleyExceptions::Tools);
	}

	// Resample
	if (needsResampling) {
		// Resample
		Logger::logWarning(asset.assetId + " requires resampling from " + toString(sampleRate) + " to 48000 Hz.");
		Concurrent::foreach(std::begin(samples), std::end(samples), [&] (std::vector<float>& s) {
			s = resampleChannel(sampleRate, 48000, s);
		});
		sampleRate = 48000;
		needsEncoding = true;
	}

	// Encode to vorbis
	if (needsEncoding) {
		encodedData = encodeVorbis(numChannels, sampleRate, samples);
		fileData = &encodedData;
		samples.clear();
	}

	// Write metadata
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("channels", numChannels);
	meta.set("sampleRate", sampleRate);

	// Output
	collector.output(asset.assetId, AssetType::AudioClip, *fileData, meta);
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
	throw Exception("Error opening Ogg Vorbis: "+str, HalleyExceptions::Tools);
}

static void writeBytes(Bytes& dst, gsl::span<const gsl::byte> src)
{
	size_t start = dst.size();
	size_t size = src.size();
	dst.resize(start + size);
	memcpy(dst.data() + start, src.data(), size);
}

static void outputPacket(Bytes& dst, ogg_packet& packet, ogg_stream_state& os, bool& eos)
{
	ogg_page og;
	ogg_stream_packetin(&os, &packet);
	while (!eos) {
		int result = ogg_stream_pageout(&os, &og);
		if (result == 0) {
			break;
		}
		writeBytes(dst, gsl::as_bytes(gsl::span<char>(reinterpret_cast<char*>(og.header), og.header_len)));
		writeBytes(dst, gsl::as_bytes(gsl::span<char>(reinterpret_cast<char*>(og.body), og.body_len)));

		if (ogg_page_eos(&og)) {
			eos = true;
		}
	}
}

Bytes AudioImporter::encodeVorbis(int nChannels, int sampleRate, gsl::span<const std::vector<float>> src)
{
	Bytes result;
	int ret = 0;
	bool eos = false;

	ogg_stream_state os;
	ogg_stream_init(&os, 0);

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
	vorbis_comment vc;
	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;
		vorbis_comment_init(&vc);
		vorbis_comment_add_tag(&vc, "ENCODER", "Halley");
		ret = vorbis_analysis_headerout(&v, &vc, &header, &header_comm, &header_code);
		if (ret) {
			onVorbisError(ret);
		}
		ogg_stream_packetin(&os, &header);
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);
		ogg_page og;
		while (!eos) {
			ret = ogg_stream_flush(&os, &og);
			if (ret == 0) {
				break;
			}
			writeBytes(result, gsl::as_bytes(gsl::span<char>(reinterpret_cast<char*>(og.header), og.header_len)));
			writeBytes(result, gsl::as_bytes(gsl::span<char>(reinterpret_cast<char*>(og.body), og.body_len)));
		}
	}

	// 4.
	vorbis_block vb;
	ogg_packet op;
	ret = vorbis_block_init(&v, &vb);
	if (ret) {
		onVorbisError(ret);
	}

	// 5. / 6.
	constexpr int bufferSize = 1024;
	size_t pos = 0;
	size_t len = src[0].size();
	while (!eos) {
		// 5.1.
		Expects(pos <= len);
		size_t samplesToWrite = std::min(len - pos, size_t(bufferSize));
		float** buffers = vorbis_analysis_buffer(&v, bufferSize);
		for (size_t i = 0; i < nChannels; ++i) {
			for (int j = 0; j < samplesToWrite; ++j) {
				buffers[i][j] = src[i][j + pos];
			}
		}
		pos += samplesToWrite;

		ret = vorbis_analysis_wrote(&v, int(samplesToWrite));
		if (ret) {
			onVorbisError(ret);
		}
		
		// 5.2.
		while (vorbis_analysis_blockout(&v, &vb) == 1) {
			// 5.2.1.
			ret = vorbis_analysis(&vb, nullptr);
			if (ret) {
				onVorbisError(ret);
			}
			
			// 5.2.2.
			ret = vorbis_bitrate_addblock(&vb);
			if (ret) {
				onVorbisError(ret);
			}

			while (vorbis_bitrate_flushpacket(&v, &op)) {
				// 5.2.3.
				outputPacket(result, op, os, eos);
			}
		}

		if (samplesToWrite == 0) {
			eos = true;
		}
	}

	// 7.
	vorbis_comment_clear(&vc);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&v);
	vorbis_info_clear(&vi);
	ogg_stream_clear(&os);

	return result;
}

std::vector<float> AudioImporter::resampleChannel(int from, int to, gsl::span<const float> src)
{
	AudioResampler resampler(from, to, 1, 1.0f);
	std::vector<float> dst(resampler.numOutputSamples(src.size()) + 1024);
	auto result = resampler.resampleInterleaved(src, dst);
	if (result.nRead != src.size()) {
		throw Exception("Only read " + toString(result.nRead) + " samples, expected " + toString(src.size()), HalleyExceptions::Tools);
	}
	if (result.nWritten == dst.size()) {
		throw Exception("Resample dst buffer overflow.", HalleyExceptions::Tools);
	}
	dst.resize(result.nWritten);
	return dst;
}
