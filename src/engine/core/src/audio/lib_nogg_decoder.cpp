#include "lib_nogg_decoder.h"

#include "halley/resources/resource_data.h"
#include "libnogg/include/nogg.h"

using namespace Halley;

namespace {
		
	void onVorbisError(int error)
	{
		String str;
		switch (error) {
		case VORBIS_ERROR_INVALID_ARGUMENT: str = "An invalid argument was passed to a function."; break;
		case VORBIS_ERROR_DISABLED_FUNCTION: str = "The requested function is not supported in this build of the library."; break;
		case VORBIS_ERROR_INSUFFICIENT_RESOURCES: str = "Insufficient system resources were available for the operation."; break;
		case VORBIS_ERROR_FILE_OPEN_FAILED: str = "vorbis_open_file() failed to open the requested file."; break;
		case VORBIS_ERROR_INVALID_OPERATION: str = "An invalid operation was attempted."; break;
		case VORBIS_ERROR_NO_CPU_SUPPORT: str = "The runtime environment does not support the CPU instruction set (including extensions) for which the library was compiled."; break;
		case VORBIS_ERROR_STREAM_INVALID: str = "The stream is not a Vorbis stream or is corrupt."; break;
		case VORBIS_ERROR_STREAM_NOT_SEEKABLE: str = "A seek operation was attempted on an unseekable stream."; break;
		case VORBIS_ERROR_STREAM_END: str = " A read operation attempted to read past the end of the stream (for a packet-submission decoder, past the end of the packet)."; break;
		case VORBIS_ERROR_DECODE_SETUP_FAILED: str = "An error occurred while initializing the Vorbis decoder."; break;
		case VORBIS_ERROR_DECODE_FAILED: str = "An unrecoverable error occurred while decoding audio data."; break;
		case VORBIS_ERROR_DECODE_RECOVERED: str = "An error was detected in the stream, but the decoder was able to recover and subsequent read operations may be attempted."; break;
		default: str = "Unknown error.";
		}
		throw Exception("Error opening Ogg Vorbis: "+str, HalleyExceptions::Resources);
	}

}

LibNoggDecoder::LibNoggDecoder(const VorbisData& data)
	: data(data)
{
	vorbis_callbacks_t callbacks;
	callbacks.read = vorbisRead;
	callbacks.close = vorbisClose;
	callbacks.seek = vorbisSeek;
	callbacks.tell = vorbisTell;
	callbacks.length = vorbisLength;
	callbacks.malloc = vorbisMalloc;
	callbacks.free = vorbisFree;

	int options = 0;

	vorbis_error_t result;
	handle = vorbis_open_callbacks(callbacks, this, options, &result);
	if (result != VORBIS_NO_ERROR) {
		onVorbisError(result);
	}
}

LibNoggDecoder::~LibNoggDecoder()
{
	if (handle) {
		vorbis_close(handle);
		handle = nullptr;
	}
}

size_t LibNoggDecoder::read(AudioMultiChannelSamples dst, size_t nChannels)
{
	size_t totalRead = 0;
	size_t toReadLeft = dst[0].size();

	std::array<float, 4 * 1024> buffer;
	size_t capacityPerRead = buffer.size() / nChannels;

	while (toReadLeft > 0) {
		vorbis_error_t error;
		int nRead = vorbis_read_float(handle, buffer.data(), int32_t(std::min(toReadLeft, capacityPerRead)), &error);
		if (error != VORBIS_NO_ERROR) {
			onVorbisError(nRead);
			return 0;
		}

		if (nRead > 0) {
			for (size_t j = 0; j < nChannels; ++j) {
				auto curDst = dst[j].subspan(totalRead);
				for (size_t i = 0; i < nRead; ++i) {
					curDst[i] = buffer[i * nChannels + j];
				}
			}

			totalRead += nRead;
			toReadLeft -= nRead;
		} else if (nRead == 0) {
			break;
		}
	}
	return totalRead;
}

size_t LibNoggDecoder::getNumSamples() const
{
	return vorbis_length(handle);
}

int LibNoggDecoder::getSampleRate() const
{
	return static_cast<int>(vorbis_rate(handle));
}

int LibNoggDecoder::getNumChannels() const
{
	return vorbis_channels(handle);
}

void LibNoggDecoder::seek(size_t sample)
{
	vorbis_seek(handle, sample);
}

size_t LibNoggDecoder::tell() const
{
	return vorbis_tell(handle);
}

int64_t LibNoggDecoder::vorbisLength(void* opaque)
{
	auto& dec = *static_cast<LibNoggDecoder*>(opaque);
	auto& data = dec.data;
	
	if (data.isStreaming()) {
		auto& res = data.getStream();
		return res->size();
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data.getResource());
		return res->getSize();
	}
}

int64_t LibNoggDecoder::vorbisTell(void* opaque)
{
	const LibNoggDecoder& dec = *static_cast<LibNoggDecoder*>(opaque);
	const VorbisData& data = dec.data;

	if (data.isStreaming()) {
		return static_cast<long>(data.getStream()->tell());
	} else {
		return static_cast<long>(dec.pos);
	}
}

void LibNoggDecoder::vorbisSeek(void* opaque, int64_t dst)
{
	LibNoggDecoder* dec = static_cast<LibNoggDecoder*>(opaque);
	auto& data = dec->data;

	if (data.isStreaming()) {
		const auto& res = data.getStream();
		res->seek(dst, SEEK_SET);

		const auto actualPos = res->tell();
		if (actualPos != dst) {
			Logger::logError("Failed to seek OggVorbis stream: tried seeking to " + toString(dst) + ", at " + toString(actualPos));
		}
	}

	dec->pos = dst;
}

int32_t LibNoggDecoder::vorbisRead(void* opaque, void* buffer, int32_t length)
{
	auto& dec = *static_cast<LibNoggDecoder*>(opaque);
	auto& data = dec.data;
	
	if (data.isStreaming()) {
		auto res = data.getStream();
		size_t requested = length;
		int r = res->readAt(as_writable_bytes(gsl::span<char>(static_cast<char*>(buffer), requested)), dec.pos);
		dec.pos += r;
		return r;
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data.getResource());
		long long totalSize = res->getSize();
		size_t left = totalSize - dec.pos;
		size_t requested = length;
		size_t toRead = std::min(requested, left);

		const char* src = static_cast<const char*>(res->getData());
		memcpy(buffer, src + dec.pos, toRead);
		dec.pos += toRead;
		return static_cast<int32_t>(toRead);
	}
}

void LibNoggDecoder::vorbisClose(void* opaque)
{
}

void* LibNoggDecoder::vorbisMalloc(void* opaque, int32_t size, int32_t align)
{
#ifdef _MSC_VER
	return _aligned_malloc(size, std::max(align, 1));
#else
	return aligned_alloc(align, size);
#endif
}

void LibNoggDecoder::vorbisFree(void* opaque, void* ptr)
{
#ifdef _MSC_VER
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}
