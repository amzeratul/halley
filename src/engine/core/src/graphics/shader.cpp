#include "halley/core/graphics/shader.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

std::unique_ptr<ShaderFile> ShaderFile::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	auto result = std::make_unique<ShaderFile>();
	s >> *result;
	return result;
}

void ShaderFile::reload(Resource resource)
{
	*this = std::move(dynamic_cast<ShaderFile&>(resource));
}

void ShaderFile::serialize(Serializer& s) const
{
	s << shaders;
}

void ShaderFile::deserialize(Deserializer& s)
{
	s >> shaders;
}
