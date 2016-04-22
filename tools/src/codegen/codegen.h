#pragma once

#include <experimental/filesystem>

namespace Halley
{
	class Codegen
	{
	public:
		static void processFolder(String inDir, String outDir);

	private:
		void addSource(std::experimental::filesystem::path path);
	};
}