#include "halley/file_formats/binary_file.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

BinaryFile::BinaryFile() {}

BinaryFile::BinaryFile(const Bytes& data)
	: data(data)
	, streaming(false)
{}

BinaryFile::BinaryFile(Bytes&& data)
	: data(data)
	, streaming(false)
{}

BinaryFile::BinaryFile(gsl::span<const gsl::byte> d)
	: streaming(false)
{
	data.resize(d.size_bytes());
	memcpy(data.data(), d.data(), d.size_bytes());
}

BinaryFile::BinaryFile(std::unique_ptr<ResourceDataStream> stream)
	: stream(std::move(stream))
	, streaming(true)
{
}

std::unique_ptr<BinaryFile> BinaryFile::loadResource(ResourceLoader& loader)
{
	bool streaming = loader.getMeta().getBool("streaming", false);
	if (streaming) {
		return std::make_unique<BinaryFile>(loader.getStream());
	} else {
		return std::make_unique<BinaryFile>(loader.getStatic()->getSpan());
	}
}

void BinaryFile::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<BinaryFile&>(resource));
}

const Bytes& BinaryFile::getBytes() const
{
	Expects(!streaming);
	return data;
}

Bytes& BinaryFile::getBytes()
{
	Expects(!streaming);
	return data;
}

gsl::span<const gsl::byte> BinaryFile::getSpan() const
{
	Expects(!streaming);
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

gsl::span<gsl::byte> BinaryFile::getSpan()
{
	Expects(!streaming);
	return gsl::as_writeable_bytes(gsl::span<Byte>(data));
}

std::shared_ptr<ResourceDataStream> BinaryFile::getStream() const
{
	Expects(streaming);
	return stream;
}
