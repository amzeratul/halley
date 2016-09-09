#pragma once
#include "halley/file/filesystem.h"
#include "halley/resources/resource_data.h"

namespace Halley
{
	class VideoAPI;
	class InputAPI;
	class HalleyAPIInternal;
	class FileRWOps;

	class SystemAPI
	{
	public:
		virtual ~SystemAPI() {}

		virtual std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start = 0, int64_t end = -1) = 0;
		virtual std::unique_ptr<ResourceDataReader> getDataReader(gsl::span<const gsl::byte> memory) = 0;
		
	private:
		friend class HalleyAPI;
		friend class Core;

		virtual bool generateEvents(VideoAPI* video, InputAPI* input) = 0;
	};
}
