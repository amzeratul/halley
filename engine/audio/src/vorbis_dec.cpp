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
#include "vorbis_dec.h"

#include <ogg/ogg.h>
#include "halley/support/exception.h"
#include "halley/resources/resource_data.h"

#ifdef WITH_IVORBIS
	#include <ivorbiscodec.h>
	#include <ivorbisfile.h>
#else
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>
#endif

using namespace Halley;

#ifdef _MSC_VER
	#pragma comment(lib, "libogg_static.lib")
	#pragma comment(lib, "libvorbis_static.lib")
	#pragma comment(lib, "libvorbisfile_static.lib")
#endif

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

Halley::VorbisData::VorbisData(std::shared_ptr<ResourceData> resource)
	: resource(resource)
	, streaming(std::dynamic_pointer_cast<ResourceDataStream>(resource))
	, pos(0)
{
	open();
}

Halley::VorbisData::~VorbisData()
{
	close();
}

void Halley::VorbisData::open()
{
	if (streaming) {
		stream = std::dynamic_pointer_cast<ResourceDataStream>(resource)->getReader();
	}

	ov_callbacks callbacks;
	callbacks.read_func = vorbisRead;
	callbacks.close_func = vorbisClose;
	callbacks.seek_func = vorbisSeek;
	callbacks.tell_func = vorbisTell;

	file = new OggVorbis_File();
	int result = ov_open_callbacks(this, file, nullptr, 0, callbacks);
	if (result != 0) onVorbisError(result);
}

void Halley::VorbisData::close()
{
	if (file) {
		ov_clear(file);
		delete file;
		file = nullptr;
	}
}

void Halley::VorbisData::reset()
{
	close();
	open();
}

void Halley::VorbisData::getData(std::vector<char>& dst, int len)
{
	// Warning: len might be -1 ("as much as you can get")

	size_t totalRead = 0;
	const size_t bufferSize = 2048;
	char buffer[bufferSize];
	
	while (len < 0 || totalRead < size_t(len)) {
		size_t n = read(buffer, bufferSize);
		if (n == 0) break;
		totalRead += n;
		size_t dstSize = dst.size();
		size_t sz = n + dstSize;
		//while (dst.capacity() < sz) dst.reserve(dst.capacity() * 2);
		dst.resize(sz);
		memcpy(dst.data() + dstSize, buffer, n);
	}
}

size_t Halley::VorbisData::read(char* dst, size_t len)
{
	Expects(file);
	int bitstream;
	size_t nRead = 0;
	while (nRead < len) {
#ifdef WITH_IVORBIS
		int r = ov_read(file, dst+nRead, int(len-nRead), &bitstream);
#else
		int r = ov_read(file, dst+nRead, int(len-nRead), 0, 2, 1, &bitstream);
#endif
		if (r > 0) nRead += size_t(r);
		else if (r == 0) break;
		else onVorbisError(r);
	}
	return nRead;
}

size_t Halley::VorbisData::getSize() const
{
	Expects(file);
	return size_t(ov_pcm_total(file, -1)) * 2;
}

int Halley::VorbisData::getFrequency() const
{
	Expects(file);
	vorbis_info *info = ov_info(file, -1);
	return info->rate;
}

int Halley::VorbisData::getChannels() const
{
	Expects(file);
	vorbis_info *info = ov_info(file, -1);
	return info->channels;
}

void Halley::VorbisData::seek(double t)
{
	Expects(file);
	ov_time_seek(file, t);
}

size_t VorbisData::vorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	VorbisData* data = static_cast<VorbisData*>(datasource);
	
	if (data->streaming) {
		auto res = data->stream;
		size_t requested = size*nmemb;
		size_t r = res->read(gsl::as_writeable_bytes(gsl::span<char>(reinterpret_cast<char*>(ptr), requested)));
		return r;
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data->resource);
		long long totalSize = res->getSize();
		size_t left = size_t(totalSize - data->pos);
		size_t requested = size*nmemb;
		size_t toRead = std::min(requested, left);

		//std::cout << "reading " << data->pos << "->" << (data->pos+toRead) << " [" << toRead << "/" << requested << "]\n";

		const char* src = static_cast<const char*>(res->getData());
		memcpy(ptr, src+data->pos, toRead);
		data->pos += toRead;
		return toRead;
	}
}

int VorbisData::vorbisSeek(void *datasource, long long offset, int whence)
{
	VorbisData* data = static_cast<VorbisData*>(datasource);

	if (data->streaming) {
		auto res = data->stream;
		res->seek(offset, whence);
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(data->resource);
		if (whence == SEEK_SET) data->pos = offset;
		else if (whence == SEEK_CUR) data->pos += offset;
		else if (whence == SEEK_END) data->pos = int64_t(res->getSize()) + offset;
	}
	
	return 0;
}

int VorbisData::vorbisClose(void * /*dataSource*/)
{
	//std::cout << "closing\n";
	//VorbisData* data = (VorbisData*) datasource;
	return 0;
}

long VorbisData::vorbisTell(void *datasource)
{
	VorbisData* data = static_cast<VorbisData*>(datasource);

	if (data->streaming) {
		auto res = data->stream;
		return static_cast<long>(res->tell());
	} else {
		return long(data->pos);
	}
}
