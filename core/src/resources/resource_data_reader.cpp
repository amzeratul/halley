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

#include "resources/resource_data_reader.h"
#include <SDL.h>

using namespace Halley;

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
	int n = static_cast<int>(SDL_RWread(fp, dst, 1, static_cast<int>(toRead)));
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

SDL_RWops* ResourceDataReaderFile::getRWOpsFromStaticData(ResourceDataStatic& data) 
{
	return SDL_RWFromConstMem(data.getData(), static_cast<int>(data.getSize()));
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
