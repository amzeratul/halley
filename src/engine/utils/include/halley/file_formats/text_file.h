#pragma once
#include "halley/text/halleystring.h"
#include <memory>
#include "halley/resources/resource.h"

namespace Halley
{
	class ResourceLoader;

	class TextFile final : public Resource
	{
	public:
		TextFile();
		explicit TextFile(String data);

		String& getData();
		const String& getData() const;

		static std::unique_ptr<TextFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::TextFile; }
		void reload(Resource&& resource) override;

	private:
		String data;
	};
}
