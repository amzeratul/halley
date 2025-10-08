#include "halley/audio/vorbis_dec.h"

struct OggVorbis_File;

namespace Halley {
	class LibVorbisDecoder : public IVorbisDecoder {
	public:
		LibVorbisDecoder(const VorbisData& data);
		~LibVorbisDecoder() override;

		size_t read(AudioMultiChannelSamples dst, size_t nChannels) override;
		size_t getNumSamples() const override;
		int getSampleRate() const override;
		int getNumChannels() const override;
		void seek(size_t sample) override;
		size_t tell() const override;

	private:
		OggVorbis_File* file = nullptr;
		size_t pos = 0;

		const VorbisData& data;
		
		static size_t vorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource);
		static int vorbisSeek(void *datasource, OggOffsetType offset, int whence);
		static int vorbisClose(void *datasource);
		static long vorbisTell(void *datasource);
	};
}
