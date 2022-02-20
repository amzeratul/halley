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

struct OggVorbis_File;

#if defined(_MSC_VER) || defined(__clang__)
using OggOffsetType = int64_t;
#else
using OggOffsetType = long int;
#endif

namespace Halley {
	class ResourceData;
	class ResourceDataReader;

	class VorbisData {
	public:
		VorbisData(std::shared_ptr<ResourceData> resource);
		~VorbisData();

		size_t read(gsl::span<Vector<float>> dst);

		size_t getNumSamples() const; // Per channel
		int getSampleRate() const;
		int getNumChannels() const;

		void close();
		void reset();
		void seek(double t);
		void seek(size_t sample);

	private:
		void open();
		static size_t vorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource);
		static int vorbisSeek(void *datasource, OggOffsetType offset, int whence);
		static int vorbisClose(void *datasource);
		static long vorbisTell(void *datasource);

		std::shared_ptr<ResourceData> resource;
		std::shared_ptr<ResourceDataReader> stream;
		OggVorbis_File* file;
		
		bool streaming;
		long long pos;
	};
}
