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

#include "resources/resource_pack.h"
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <iostream>
#include <SDL.h>
#include <halley/support/exception.h>
#include "resources/resource_data_reader.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

namespace Halley {
	static const char* IDENTIFIER="HLYRSPCK";

	ResourcePack::ResourcePack(String name)
		: priority(0)
		, fileP(nullptr)
	{
		if (name != "") {
			load(name);
		}
	}

	ResourcePack::~ResourcePack()
	{
		if (fileP) {
			fclose(fileP);
			fileP = nullptr;
		}
	}

	void ResourcePack::add(String diskPath, String file, bool encrypted, bool zipped)
	{
		ResourcePackEntry entry;
		entry.diskPath = diskPath;
		entry.name = file;
		entry.pos = 0;
		entry.size = 0;
		entry.sizeOnDisk = 0;
		entry.encrypted = encrypted;
		entry.zipped = zipped;
		entries[entry.name] = entry;
	}

	void ResourcePack::save(String path)
	{
		FILE* fp = fopen(path.c_str(), "wb");
		if (!fp) throw Exception("Unable to open destination file");

		// Write file header
		writeHeader(fp);

		// Store TOC start position
		size_t tocStart = ftell(fp);
		writeTOC(fp);

		// Write all files
		for (auto iter = entries.begin(); iter != entries.end(); iter++) {
			writeFile(fp, iter->second);
		}

		// Rewrite TOC, now that we have more data about each file
		fseek(fp, (long)tocStart, SEEK_SET);
		writeTOC(fp);

		// Done
		fclose(fp);
	}

	void ResourcePack::writeHeader(FILE* fp)
	{
		fwrite(IDENTIFIER, 1, 8, fp);

		int version = 1;
		fwrite(&version, 4, 1, fp);
		fwrite(&priority, 4, 1, fp);
	}

	void ResourcePack::writeTOC(FILE* fp)
	{
		// Number of elements
		char buffer[1024];
		int elems = (int)entries.size();
		fwrite(&elems, 4, 1, fp);

		for (auto iter = entries.begin(); iter != entries.end(); iter++) {
			ResourcePackEntry& entry = iter->second;

			// Name
			int len = (int)entry.name.length();
			fwrite(&len, 4, 1, fp);
			strcpy(buffer, entry.name.c_str());
			encrypt(buffer, len);
			fwrite(buffer, 1, len, fp);

			// Pos and sizes
			fwrite(&entry.pos, 4, 1, fp);
			fwrite(&entry.size, 4, 1, fp);
			fwrite(&entry.sizeOnDisk, 4, 1, fp);

			// Flags
			int flags = 0;
			flags |= entry.encrypted ? 1 : 0;
			flags |= entry.zipped ? 2 : 0;
			fwrite(&flags, 4, 1, fp);
		}
	}

	void ResourcePack::writeFile(FILE* fp, ResourcePackEntry& entry)
	{
		std::cout << "Packing " << entry.name;

		try {

			// Current position is where it'll be
			entry.pos = ftell(fp);

			// Open
			FILE* fp2 = fopen(entry.diskPath.c_str(), "rb");
			if (!fp2) throw Exception("Unable to open file: "+entry.diskPath);

			// Figure out file size
			fseek(fp2, 0, SEEK_END);
			size_t len = ftell(fp2);
			fseek(fp2, 0, SEEK_SET);
			entry.size = len;
			entry.sizeOnDisk = len;

			// Read
			char* buf = new char[len];
			if (fread(buf, 1, len, fp2) != len) throw Exception("Unable to read file");
			fclose(fp2);

			// Encrypt
			if (entry.encrypted) encrypt(buf, len);

			// Write
			if (fwrite(buf, 1, len, fp) != len) throw Exception("Unable to write file");

			std::cout << " [OK]" << std::endl;
		} catch (...) {
			std::cout << " [Error]" << std::endl;
			throw;
		}
	}

	void ResourcePack::load(String path)
	{
		std::cout << "Loading resource pack: " << path << std::endl;
		name = path;
		FILE* fp = fopen(path.c_str(), "rb");
		if (!fp) throw Exception("Unable to open resource pack: "+path);

		readHeader(fp);
		readTOC(fp);

		fileP = fp;
	}

	void ResourcePack::readHeader(FILE* fp)
	{
		// Read format
		char buf[10];
		fread(buf, 1, 8, fp);
		buf[8] = 0;
		if (strncmp(buf, IDENTIFIER, 8) != 0) throw Exception("Invalid file format");

		// Read version
		int version;
		fread(&version, 4, 1, fp);
		// TODO
		
		// Read priority
		fread(&priority, 4, 1, fp);
	}
	
	void ResourcePack::readTOC(FILE* fp)
	{
		int elems;
		fread(&elems, 4, 1, fp);
		char buffer[1024];

		for (int i=0; i<elems; i++) {
			ResourcePackEntry entry;

			// Read file name
			int len;
			fread(&len, 4, 1, fp);
			if (len >= 1024) throw Exception("Filename too long!");
			fread(buffer, 1, len, fp);
			buffer[len] = 0;
			decrypt(buffer, len);
			entry.name = buffer;

			// Pos and sizes
			// (Initialize first, as they are size_t, which are 64-bit on some systems)
			entry.pos = 0;
			entry.size = 0;
			entry.sizeOnDisk = 0;
			fread(&entry.pos, 4, 1, fp);
			fread(&entry.size, 4, 1, fp);
			fread(&entry.sizeOnDisk, 4, 1, fp);

			// Flags
			int flags = 0;
			fread(&flags, 4, 1, fp);
			entry.encrypted = (flags & 1) != 0;
			entry.zipped = (flags & 2) != 0;

			entries[entry.name] = entry;
		}
	}

	std::unique_ptr<ResourceData> ResourcePack::doGet(String resource, bool stream)
	{
		auto iter = entries.find(resource);
		if (iter == entries.end()) throw Exception("Resource not found: "+resource);

		ResourcePackEntry& entry = iter->second;
		size_t sz = entry.size;
		String path = name+"/"+resource;

		if (stream) {
			if (entry.encrypted) throw Exception("Cannot stream from encrypted entry: "+resource);
			size_t start = entry.pos;

			return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
				SDL_RWops* fp = SDL_RWFromFile(name.c_str(), "rb");
				return std::make_unique<ResourceDataReaderFile>(fp, static_cast<int>(start), static_cast<int>(start+sz), true);
			});
		}

		else {
			if (sz > 100 * 1024 * 1024) {
				std::cout << "Loading resource larger than 100 MB: " << resource << " is " << sz << " bytes long." << std::endl;
			}
			char* buf = new char[sz];

			// Read from file
			fseek(fileP, (long)entry.pos, SEEK_SET);
			fread(buf, 1, sz, fileP);

			// Decrypt
			if (entry.encrypted) decrypt(buf, sz);

			return std::make_unique<ResourceDataStatic>(buf, sz, path);
		}
	}

	std::vector<String> ResourcePack::getResourceList()
	{
		std::vector<String> result;
		for (auto iter = entries.begin(); iter != entries.end(); iter++) {
			result.push_back(iter->first);
		}
		return result;
	}


	// "Encrypt" might be a bit of an exageration, it's more of just an obfuscation scheme
	const static int ENC_PRIME = 761;
	const static char ENC_MASK = -1;

	void ResourcePack::encrypt(char* data, size_t size)
	{
		char last = 0;
		for (size_t i=0; i<size; i++) {
			data[i] = char((data[i] + (last+(i*ENC_PRIME))) ^ ENC_MASK);
			last = data[i];
		}
	}

	void ResourcePack::decrypt(char* data, size_t size)
	{
		char last = 0;
		for (size_t i=0; i<size; i++) {
			char tmp = data[i];
			data[i] = char((data[i] ^ ENC_MASK) - (last+(i*ENC_PRIME)));
			last = tmp;
		}
	}

	std::time_t ResourcePack::doGetTimestamp(String resource)
	{
		return 0;
	}

}
