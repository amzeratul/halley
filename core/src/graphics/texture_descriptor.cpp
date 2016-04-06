#include "texture_descriptor.h"

int Halley::TextureDescriptor::getBitsPerPixel(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGBA:
		return 4;
	default:
		return 3;
	}
}
