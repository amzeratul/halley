#pragma once

#include "halley/data_structures/maybe.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class IClipboard
	{
	public:
		virtual ~IClipboard() = default;
		
		virtual void setData(const String& stringData) = 0;
		virtual Maybe<String> getStringData() = 0;
	};
}
