#include "httplib_web_api.h"
#include <halley/plugin/plugin.h>
#include <halley/text/halleystring.h>

namespace Halley {
	
	class HTTPLibWebPlugin : public Plugin {
	public:
		HTTPLibWebPlugin() {}
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new HTTPLibWebAPI(); }
		PluginType getType() override { return PluginType::WebAPI; }
		String getName() override { return "Web/HTTPLib"; }
	};
	
}

void initHTTPLibPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::HTTPLibWebPlugin>());
}
