#pragma once

#include <halley/text/halleystring.h>
#include "halley/maths/uuid.h"

namespace Halley {
	class Path;

    class ProjectProperties {
    public:
		ProjectProperties(Path propertiesFile);

        const UUID& getUUID() const;
    	
		const String& getName() const;
		void setName(String name);

    	const String& getAssetPackManifest() const;
    	void setAssetPackManifest(String manifest);

    	const std::vector<String>& getPlatforms() const;
    	void setPlatforms(std::vector<String> platforms);

        const String& getBinName() const;
        void setBinName(String binName);
    	
		bool getImportByExtension() const;
    	void setImportByExtension(bool enabled);

    	void setDefaultZoom(float zoom);
		float getDefaultZoom() const;

	private:
		const Path& propertiesFile;
    	UUID uuid;
		String name;
    	String assetPackManifest;
        String binName;
    	bool importByExtension = false;
    	float defaultZoom = 1.0f;
    	std::vector<String> platforms;

    	bool dirty = false;

		void load();
		void save();
    	void loadDefaults();
    };
}
