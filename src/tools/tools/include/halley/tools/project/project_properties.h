#pragma once

#include <halley/text/halleystring.h>

namespace Halley {
	class Path;

    class ProjectProperties {
    public:
		ProjectProperties(const Path& propertiesFile);

		const String& getName() const;
		void setName(String name);

    	const String& getAssetPackManifest() const;
    	void setAssetPackManifest(String manifest);

    	const std::vector<String>& getPlatforms() const;
    	void setPlatforms(std::vector<String> platforms);

        const String& getDLL() const;
        void setDLL(String dll);

	private:
		const Path& propertiesFile;
		String name;
    	String assetPackManifest;
        String dll;
    	std::vector<String> platforms;

		void load();
		void save();
    };
}
