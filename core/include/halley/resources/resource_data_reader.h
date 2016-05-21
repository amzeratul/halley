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

struct SDL_RWops;

namespace Halley {
	class ResourceDataReaderFile : public ResourceDataReader {
	public:
		ResourceDataReaderFile(SDL_RWops* fp, int start, int end, bool closeOnFinish);
		int read(void* dst, size_t size) override;
		void seek(long long pos, int whence) override;
		size_t tell() override;
		void close() override;

		static SDL_RWops* getRWOpsFromStaticData(ResourceDataStatic& data);

	private:
		SDL_RWops* fp;
		int pos;
		int start;
		int end;
		bool closeOnFinish;
	};
}
