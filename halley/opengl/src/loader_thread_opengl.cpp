#include "loader_thread_opengl.h"
#include "halley/core/api/system_api.h"
#include "halley_gl.h"
#include "halley/core/halley_core.h"

using namespace Halley;

LoaderThreadOpenGL::LoaderThreadOpenGL(GLContext& context)
	: parentContext(context)
	, executor(Executors::getVideoAux())
{
	workerThread = std::thread([this]() { run(); });
}

LoaderThreadOpenGL::~LoaderThreadOpenGL()
{	
	executor.stop();
	workerThread.join();
	context.reset();
}

std::thread::id LoaderThreadOpenGL::getThreadId()
{
	return workerThread.get_id();
}

void LoaderThreadOpenGL::run()
{
	Concurrent::setThreadName("OpenGL loader");
	initializeContext();
	executor.runForever();
}

void LoaderThreadOpenGL::initializeContext()
{
	context = parentContext.createSharedContext();
	context->bind();
}

void LoaderThreadOpenGL::waitForGPU()
{
	// See https://forums.libsdl.org/viewtopic.php?t=9036&sid=3374c819e18df779e17b4ce5a49fdd15
	GLsync fenceId = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	while (glClientWaitSync(fenceId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(5000000000)) == GL_TIMEOUT_EXPIRED) {}
	glDeleteSync(fenceId);
}
