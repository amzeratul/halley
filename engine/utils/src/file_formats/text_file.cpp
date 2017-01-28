#include "halley/file_formats/text_file.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

TextFile::TextFile() {}

TextFile::TextFile(const String& data)
	: data(data)
{}

TextFile::TextFile(String&& data)
	: data(data)
{}

std::unique_ptr<TextFile> TextFile::loadResource(ResourceLoader& loader)
{
	return std::make_unique<TextFile>(loader.getStatic()->getString());
}
