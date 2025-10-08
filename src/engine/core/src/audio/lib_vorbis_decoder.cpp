#include "lib_vorbis_decoder.h"

#include "halley/resources/resource_data.h"

#if defined(WITH_IVORBIS)
	#include "ivorbiscodec.h"
	#include "ivorbisfile.h"
#else
	#include "libvorbis/include/vorbis/codec.h"
	#include "libvorbis/include/vorbis/vorbisfile.h"
#endif

using namespace Halley;

namespace {
	void onVorbisError(int error)
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
		throw Exception("Error opening Ogg Vorbis: "+str, HalleyExceptions::Resources);
	}
}

LibVorbisDecoder::LibVorbisDecoder(const VorbisData& data)
	: data(data)
{
	ov_callbacks callbacks;
	callbacks.read_func = vorbisRead;
	callbacks.close_func = vorbisClose;
	callbacks.seek_func = vorbisSeek;
	callbacks.tell_func = vorbisTell;

	handle = new OggVorbis_File();
	int result = ov_open_callbacks(this, handle, nullptr, 0, callbacks);
	if (result != 0) {
		onVorbisError(result);
	}
}

LibVorbisDecoder::~LibVorbisDecoder()
{
	if (handle) {
		ov_clear(handle);
		delete handle;
		handle = nullptr;
	}
}

size_t LibVorbisDecoder::read(AudioMultiChannelSamples dst, size_t nChannels)
{
	int bitstream;
	size_t totalRead = 0;
	size_t toReadLeft = dst[0].size();

	while (toReadLeft > 0) {
		float **pcm;
		int nRead = ov_read_float(handle, &pcm, int(toReadLeft), &bitstream);
		if (nRead < 0) {
			onVorbisError(nRead);
		}

		if (nRead > 0) {
			for (size_t i = 0; i < nChannels; ++i) {
				memcpy(dst[i].data() + totalRead, pcm[i], nRead * sizeof(float));
			}

			totalRead += nRead;
			toReadLeft -= nRead;
		} else if (nRead == 0) {
			break;
		}
	}
	return totalRead;
}

size_t LibVorbisDecoder::getNumSamples() const
{
	Expects(handle);
	return size_t(ov_pcm_total(handle, -1));
}

int LibVorbisDecoder::getSampleRate() const
{
	Expects(handle);
	vorbis_info *info = ov_info(handle, -1);
	return info->rate;
}

int LibVorbisDecoder::getNumChannels() const
{
	Expects(handle);
	vorbis_info *info = ov_info(handle, -1);
	return info->channels;
}

void LibVorbisDecoder::seek(size_t sample)
{
	ov_pcm_seek(handle, static_cast<ogg_int64_t>(sample));
}

size_t LibVorbisDecoder::tell() const
{
	if (handle) {
		return ov_pcm_tell(handle);
	} else {
		return 0;
	}
}

size_t LibVorbisDecoder::vorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	auto& dec = *static_cast<LibVorbisDecoder*>(datasource);
	auto& data = dec.data;
	
	if (data.isStreaming()) {
		auto res = data.getStream();
		size_t requested = size*nmemb;
		size_t r = res->readAt(as_writable_bytes(gsl::span<char>(static_cast<char*>(ptr), requested)), dec.pos);
		dec.pos += r;
		return r;
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data.getResource());
		long long totalSize = res->getSize();
		size_t left = totalSize - dec.pos;
		size_t requested = size*nmemb;
		size_t toRead = std::min(requested, left);

		const char* src = static_cast<const char*>(res->getData());
		memcpy(ptr, src + dec.pos, toRead);
		dec.pos += toRead;
		return toRead;
	}
}

int LibVorbisDecoder::vorbisSeek(void *datasource, OggOffsetType offset, int whence)
{
	LibVorbisDecoder* dec = static_cast<LibVorbisDecoder*>(datasource);
	auto& data = dec->data;

	int64_t dst = 0;
	if (whence == SEEK_SET) {
		dst = offset;
	} else if (whence == SEEK_CUR) {
		dst = dec->pos + offset;
	} else if (whence == SEEK_END) {
		if (!data.isStreaming()) {
			if (auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data.getResource())) {
				dst = dec->pos + static_cast<int64_t>(res->getSize()) + offset;
			}
		}
	}

	if (data.isStreaming()) {
		const auto& res = data.getStream();
		res->seek(offset, whence);

		if (whence == SEEK_END) {
			dst = res->tell();
		} else {
			const auto actualPos = res->tell();
			if (actualPos != dst) {
				Logger::logError("Failed to seek OggVorbis stream: tried seeking to " + toString(dst) + ", at " + toString(actualPos));
			}
		}
	}

	dec->pos = dst;
	
	return 0;
}

int LibVorbisDecoder::vorbisClose(void *)
{
	return 0;
}

long LibVorbisDecoder::vorbisTell(void *datasource)
{
	const LibVorbisDecoder& dec = *static_cast<LibVorbisDecoder*>(datasource);
	const VorbisData& data = dec.data;

	if (data.isStreaming()) {
		return static_cast<long>(data.getStream()->tell());
	} else {
		return static_cast<long>(dec.pos);
	}
}
