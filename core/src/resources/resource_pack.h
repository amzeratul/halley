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

#include "resource_locator.h"

namespace Halley {
	class ResourcePackEntry {
	public:
		String name;
		String diskPath;
		size_t size;
		size_t sizeOnDisk;
		size_t pos;

		bool encrypted;
		bool zipped;
	};

	class ResourcePack : public IResourceLocatorProvider {
	public:
		ResourcePack(String name="");
		~ResourcePack();

		void add(String root, String file, bool encrypted=false, bool zipped=false);

		void save(String path);
		void load(String path);

	protected:
		std::unique_ptr<ResourceData> doGet(String resource, bool stream) override;
		std::time_t doGetTimestamp(String resource) override;
		std::vector<String> getResourceList() override;
		int getPriority() const { return priority; }

	private:

		void writeHeader(FILE* fp);
		void writeTOC(FILE* fp);
		void writeFile(FILE* fp, ResourcePackEntry& entry);

		void readHeader(FILE* fp);
		void readTOC(FILE* fp);

		static void encrypt(char* data, size_t size);
		static void decrypt(char* data, size_t size);

		std::map<String, ResourcePackEntry> entries;
		int priority;
		FILE* fileP;
		String name;
	};
}
