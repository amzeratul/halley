#pragma once

namespace Halley
{
	namespace MainWrapper
	{
		template <typename T>
		int main(int argc, char* argv[])
		{
			try {
				StringArray args;
				for (int i = 0; i < argc; i++) {
					args.push_back(argv[i]);
				}
				Core runner(std::make_unique<T>(), args);
				return 0;
			} catch (std::exception& e) {
				std::cout << "Unhandled std::exception: " << e.what() << std::endl;
				return 1;
			} catch (...) {
				std::cout << "Unhandled unknown exception." << std::endl;
				return 1;
			}
		}
	}
}
