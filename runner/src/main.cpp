#include "dynamic_loader.h"
#include <halley/runner/halley_main.h>
#include <boost/filesystem.hpp>

using namespace Halley;

int main(int argc, char** argv) {
	StringArray args;
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	using namespace boost::filesystem;
	path p(args[0].cppStr());

	DynamicGameLoader loader(p.parent_path().string() + "/halley-sample-test");

	return HalleyMain::runMain(loader, args);
}
