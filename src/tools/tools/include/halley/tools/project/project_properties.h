#pragma once

#include <halley/text/halleystring.h>
#include "halley/maths/uuid.h"
#include "halley/text/i18n.h"

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

    	const Vector<String>& getPlatforms() const;
    	void setPlatforms(Vector<String> platforms);

        const String& getBinName() const;
        void setBinName(String binName);
    	
		bool getImportByExtension() const;
    	void setImportByExtension(bool enabled);

    	void setDefaultZoom(float zoom);
		float getDefaultZoom() const;

		const I18NLanguage& getOriginalLanguage() const;
		const Vector<I18NLanguage>& getLanguages() const;

	private:
		const Path& propertiesFile;
    	UUID uuid;
		String name;
    	String assetPackManifest;
        String binName;
        I18NLanguage originalLanguage;
        Vector<I18NLanguage> languages;
    	bool importByExtension = false;
    	float defaultZoom = 1.0f;
    	Vector<String> platforms;

    	bool dirty = false;

		void load();
		void save();
    	void loadDefaults();
    };
}
