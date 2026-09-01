#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class FontImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Font; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		Vector<char32_t> getCharactersToImport(const Metadata& meta, IAssetCollector& collector) const;
		void getCharactersForScript(std::set<char32_t>& charsOutput, const String& script, IAssetCollector& collector) const;
		void addCharactersFromFile(std::set<char32_t>& output, const String& filename, IAssetCollector& collector) const;
		void addRange(std::set<char32_t>& output, char32_t codePointStart, char32_t codePointEnd) const;
		void addList(std::set<char32_t>& output, gsl::span<const char32_t> chars) const;
	};
}
