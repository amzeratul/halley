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

#include "resource_data.h"
#include <SDL.h>

using namespace Halley;


Halley::ResourceData::ResourceData(String p)
	: path(p)
{
}

ResourceDataStatic::ResourceDataStatic(String path)
	: ResourceData(path)
	, loaded(false)
{
}

ResourceDataStatic::ResourceDataStatic(void* _data, size_t _size, String path)
	: ResourceData(path)
	, loaded(false)
{
	set(_data, _size);
}

SDL_RWops* ResourceDataStatic::getRWOps()
{
	return SDL_RWFromConstMem(data.get(), (int)size);
}

static void deleter(char* data)
{
	delete[] data;
}

void Halley::ResourceDataStatic::set(void* _data, size_t _size)
{
	data = shared_ptr<char>((char*) _data, deleter);
	size = _size;
	loaded = true;
}

void* Halley::ResourceDataStatic::getData()
{
	if (!loaded) throw Exception("Resource data not yet loaded");
	return data.get();
}

const void* Halley::ResourceDataStatic::getData() const
{
	if (!loaded) throw Exception("Resource data not yet loaded");
	return data.get();
}

size_t Halley::ResourceDataStatic::getSize() const
{
	if (!loaded) throw Exception("Resource data not yet loaded");
	return size;
}

bool Halley::ResourceDataStatic::isLoaded()
{
	return loaded;
}

Halley::String Halley::ResourceDataStatic::getString() const
{
	return String((const char*)getData(), getSize());
}

Halley::ResourceDataStream::ResourceDataStream(String path, ResourceDataMakeReader makeReader)
	: ResourceData(path)
	, make(makeReader)
{
}



Halley::ResourceDataReaderFile::ResourceDataReaderFile(SDL_RWops* _fp, int _start, int _end, bool _closeOnFinish)
	: fp(_fp)
	, pos(_start)
	, start(_start)
	, end(_end)
	, closeOnFinish(_closeOnFinish)
{
	assert (fp);
}

int Halley::ResourceDataReaderFile::read(void* dst, size_t size)
{
	if (!fp) return -1;

	size_t toRead = std::min(size, size_t(end-pos));
	SDL_RWseek(fp, pos, SEEK_SET);
	int n = (int)SDL_RWread(fp, dst, 1, (int)toRead);
	if (n > 0) pos += n;
	return n;
}

void Halley::ResourceDataReaderFile::close()
{
	if (fp) {
		if (closeOnFinish) SDL_RWclose(fp);
		fp = nullptr;
		pos = end;
	}
}

void Halley::ResourceDataReaderFile::seek(long long offset, int whence)
{
	if (whence == SEEK_SET) pos = int(offset + start);
	else if (whence == SEEK_CUR) pos += int(offset);
	else if (whence == SEEK_END) pos = int(end + offset);
	SDL_RWseek(fp, pos, SEEK_SET);
}

size_t Halley::ResourceDataReaderFile::tell()
{
	return pos - start;
}
