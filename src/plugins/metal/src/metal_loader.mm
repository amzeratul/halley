#include "metal_loader.h"

using namespace Halley;

MetalLoader::MetalLoader(SystemAPI& system)
	: executor(Executors::getVideoAux())
{
	thread = system.createThread("Metal Loader", ThreadPriority::Normal, [this]() {
		executor.runForever();
	});
}

MetalLoader::~MetalLoader() {
	executor.stop();
	thread.join();
}
