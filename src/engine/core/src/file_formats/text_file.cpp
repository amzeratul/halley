#include "halley/file_formats/text_file.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

TextFile::TextFile() {}

TextFile::TextFile(String data)
	: data(std::move(data))
{}

String& TextFile::getData()
{
	return data;
}

const String& TextFile::getData() const
{
	return data;
}

std::unique_ptr<TextFile> TextFile::loadResource(ResourceLoader& loader)
{
	return std::make_unique<TextFile>(loader.getStatic()->getString());
}

void TextFile::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<TextFile&>(resource));
}
