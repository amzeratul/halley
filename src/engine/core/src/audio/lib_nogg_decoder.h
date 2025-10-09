#ifdef WITH_LIBNOGG

#include "halley/audio/vorbis_dec.h"

typedef struct vorbis_t vorbis_t;

namespace Halley {
	class LibNoggDecoder : public IVorbisDecoder {
	public:
		LibNoggDecoder(const VorbisData& data);
		~LibNoggDecoder() override;

		size_t read(AudioMultiChannelSamples dst, size_t nChannels) override;
		size_t getNumSamples() const override;
		int getSampleRate() const override;
		int getNumChannels() const override;
		void seek(size_t sample) override;
		size_t tell() const override;

	private:
		const VorbisData& data;

		vorbis_t* handle = nullptr;
		size_t pos = 0;

		static int64_t vorbisLength(void* opaque);
		static int64_t vorbisTell(void* opaque);
		static void vorbisSeek(void* opaque, int64_t offset);
		static int32_t vorbisRead(void* opaque, void* buffer, int32_t length);
		static void vorbisClose(void* opaque);
		static void* vorbisMalloc(void* opaque, int32_t size, int32_t align);
		static void vorbisFree(void* opaque, void* ptr);
	};
}

#endif
