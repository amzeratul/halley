#include "resource_data.h"
#include <SDL.h>
#include "../debug/exception.h"

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

static void deleter(char* data)
{
	delete[] data;
}

void Halley::ResourceDataStatic::set(void* _data, size_t _size)
{
	data = std::shared_ptr<char>(static_cast<char*>(_data), deleter);
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

bool Halley::ResourceDataStatic::isLoaded() const
{
	return loaded;
}

Halley::String Halley::ResourceDataStatic::getString() const
{
	return String(static_cast<const char*>(getData()), getSize());
}

Halley::ResourceDataStream::ResourceDataStream(String path, ResourceDataMakeReader makeReader)
	: ResourceData(path)
	, make(makeReader)
{
}
