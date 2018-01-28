#include "dx11_loader.h"
#include "dx11_video.h"
using namespace Halley;

DX11Loader::DX11Loader(DX11Video& video)
	: executor(Executors::getVideoAux())
{
	thread = video.getSystem().createThread("DX11 Loader", [this] ()
	{
		executor.runForever();
	});
}

DX11Loader::~DX11Loader()
{
	executor.stop();
	thread.join();
}
