/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <memory>
#include "halley/data_structures/vector.h"
#include <gsl/gsl>

#include "halley/api/audio_api.h"

#if defined(_MSC_VER) || defined(__clang__)
using OggOffsetType = int64_t;
#else
using OggOffsetType = long int;
#endif

#define WITH_LIBNOGG

namespace Halley {
	class IVorbisDecoder;
	class ResourceData;
	class ResourceDataReader;

	class VorbisData {
	public:
		VorbisData(std::shared_ptr<ResourceData> resource, bool open);
		~VorbisData();

		size_t read(gsl::span<Vector<float>> dst);
		size_t read(AudioMultiChannelSamples dst, size_t nChannels);

		size_t getNumSamples() const; // Per channel
		int getSampleRate() const;
		int getNumChannels() const;

		void close();
		void reset();
		void seek(size_t sample);
		size_t tell() const;

		size_t getSizeBytes() const;

		bool isStreaming() const;
		const std::shared_ptr<ResourceData>& getResource() const;
		const std::shared_ptr<ResourceDataReader>& getStream() const;

	private:
		void open();

		std::shared_ptr<ResourceData> resource;
		std::shared_ptr<ResourceDataReader> stream;
		std::unique_ptr<IVorbisDecoder> decoder;

		bool streaming;
		bool error = false;
	};

	class IVorbisDecoder {
	public:
		virtual ~IVorbisDecoder() = default;

		virtual size_t read(AudioMultiChannelSamples dst, size_t nChannels) = 0;
		virtual size_t getNumSamples() const = 0; // Per channel
		virtual int getSampleRate() const = 0;
		virtual int getNumChannels() const = 0;
		virtual void seek(size_t sample) = 0;
		virtual size_t tell() const = 0;
	};
}
