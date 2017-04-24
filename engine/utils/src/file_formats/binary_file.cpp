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

const Bytes& BinaryFile::getBytes() const
{
	return data;
}

Bytes& BinaryFile::getBytes()
{
	return data;
}

gsl::span<const gsl::byte> BinaryFile::getSpan() const
{
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

gsl::span<gsl::byte> BinaryFile::getSpan()
{
	return gsl::as_writeable_bytes(gsl::span<Byte>(data));
}
