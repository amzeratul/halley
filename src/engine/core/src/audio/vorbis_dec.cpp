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

#include "halley/utils/utils.h"
#include "halley/audio/vorbis_dec.h"

#include "halley/resources/resource_data.h"
#include "halley/support/assert.h"

#include "lib_nogg_decoder.h"
#include "lib_vorbis_decoder.h"

using namespace Halley;


VorbisData::VorbisData(std::shared_ptr<ResourceData> resource, bool doOpen)
	: resource(resource)
	, decoder(nullptr)
	, streaming(std::dynamic_pointer_cast<ResourceDataStream>(resource))
{
	if (doOpen) {
		open();
	}
}

VorbisData::~VorbisData()
{
	close();
}

void VorbisData::open()
{
	close();

	if (stream && !stream->isAvailable()) {
		error = true;
		return;
	}

	if (error) {
		return;
	}

	if (streaming) {
		stream = std::dynamic_pointer_cast<ResourceDataStream>(resource)->getReader();
	}

#ifdef WITH_LIBNOGG
	decoder = std::make_unique<LibNoggDecoder>(*this);
#else
	decoder = std::make_unique<LibVorbisDecoder>(*this);
#endif
}

void VorbisData::close()
{
	decoder = {};
}

void VorbisData::reset()
{
	close();
	open();
}

size_t VorbisData::read(gsl::span<Vector<float>> dst)
{
	AudioMultiChannelSamples samplesSpan;
	for (size_t i = 0; i < dst.size(); ++i) {
		samplesSpan[i] = dst[i];
	}
	return read(samplesSpan, dst.size());
}

size_t VorbisData::read(AudioMultiChannelSamples dst, size_t nChannels)
{
	if (!decoder) {
		open();
	}

	if (stream && !stream->isAvailable()) {
		error = true;
		close();
	}

	if (!decoder || error) {
		return 0;
	}

	HalleyAssertDev(nChannels == getNumChannels());

	return decoder->read(dst, nChannels);
}

size_t VorbisData::getNumSamples() const
{
	if (!decoder || error || (stream && !stream->isAvailable())) {
		return 0;
	}

	return decoder->getNumSamples();
}

int VorbisData::getSampleRate() const
{
	if (!decoder || error || (stream && !stream->isAvailable())) {
		return 0;
	}

	return decoder->getSampleRate();
}

int VorbisData::getNumChannels() const
{
	if (error || (stream && !stream->isAvailable())) {
		return 0;
	}

	return decoder->getNumChannels();
}

void VorbisData::seek(size_t sample)
{
	if (!decoder) {
		open();
	}
	if (error) {
		return;
	}
	decoder->seek(sample);
}

size_t VorbisData::tell() const
{
	return decoder ? decoder->tell() : 0;
}

size_t VorbisData::getSizeBytes() const
{
	if (streaming) {
		return sizeof(ResourceDataStream);
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(resource);
		return res->getSize() + sizeof(ResourceDataStatic);
	}
}

bool VorbisData::isStreaming() const
{
	return streaming;
}

const std::shared_ptr<ResourceData>& VorbisData::getResource() const
{
	return resource;
}

const std::shared_ptr<ResourceDataReader>& VorbisData::getStream() const
{
	return stream;
}
