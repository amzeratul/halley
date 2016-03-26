#pragma once

namespace Halley
{
	namespace MainWrapper
	{
		template <typename T>
		int main(int argc, char* argv[])
		{
			try {
				Halley::StringArray args;
				for (int i = 0; i < argc; i++) {
					args.push_back(argv[i]);
				}
				CoreRunner runner;
				return runner.run(std::make_unique<T>(), args);
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
