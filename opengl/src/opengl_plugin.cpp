#include "video_opengl.h"

namespace Halley {
	
	class OpenGLPlugin : public Plugin {
		HalleyAPIInternal* createAPI() override { return new VideoOpenGL();	}
		PluginType getType() override { return PluginType::GraphicsAPI; }
		String getName() override { return "Video/OpenGL"; }
	};
	
}

void initOpenGLPlugin()
{
	Halley::Plugin::registerPlugin(std::make_unique<Halley::OpenGLPlugin>());
}
