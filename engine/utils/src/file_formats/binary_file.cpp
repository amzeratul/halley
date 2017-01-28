#include "halley/file_formats/binary_file.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

BinaryFile::BinaryFile() {}

BinaryFile::BinaryFile(const Bytes& data)
	: data(data)
{}

BinaryFile::BinaryFile(Bytes&& data)
	: data(data)
{}

BinaryFile::BinaryFile(gsl::span<const gsl::byte> d)
{
	data.resize(d.size_bytes());
	memcpy(data.data(), d.data(), d.size_bytes());
}

std::unique_ptr<BinaryFile> BinaryFile::loadResource(ResourceLoader& loader)
{
	return std::make_unique<BinaryFile>(loader.getStatic()->getSpan());
}
