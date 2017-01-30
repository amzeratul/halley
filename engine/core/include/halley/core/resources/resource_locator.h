/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <ctime>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>
#include <halley/data_structures/vector.h>

namespace Halley {
	class ResourceData;
	class SystemAPI;

	class IResourceLocatorProvider {
		friend class ResourceLocator;

	public:
		virtual ~IResourceLocatorProvider() {}

	protected:
		virtual std::unique_ptr<ResourceData> doGet(const String& resource, bool stream)=0;
		virtual Vector<String> getResourceList()=0;
		virtual int getPriority() const { return 0; }
	};

	class ResourceLocator : public IResourceLocator
	{
	public:
		explicit ResourceLocator(SystemAPI& system);
		void add(std::unique_ptr<IResourceLocatorProvider> locator);
		void addFileSystem(Path path);
		
		std::unique_ptr<ResourceData> getResource(const String& resource, bool stream);
		std::unique_ptr<ResourceData> tryGetResource(const String& resource, bool stream);
		std::unique_ptr<ResourceDataStatic> getStatic(const String& resource) override;
		std::unique_ptr<ResourceDataStream> getStream(const String& resource) override;
		StringArray enumerate(String prefix = "", bool removePrefix = false, String suffixMatch = "");

	private:
		SystemAPI& system;
		HashMap<String, IResourceLocatorProvider*> locators;
		Vector<std::unique_ptr<IResourceLocatorProvider>> locatorList;
	};
}
