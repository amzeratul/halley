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

	class IResourceLocatorProvider {
		friend class ResourceLocator;

	public:
		virtual ~IResourceLocatorProvider() {}

	protected:
		virtual std::unique_ptr<ResourceData> doGet(String resource, bool stream)=0;
		virtual std::time_t doGetTimestamp(String resource) = 0;
		virtual Vector<String> getResourceList()=0;
		virtual int getPriority() { return 0; }
	};

	class ResourceLocator : public IResourceLocator
	{
	public:
		void add(std::unique_ptr<IResourceLocatorProvider> locator);
		void addFileSystem(String path);
		
		std::unique_ptr<ResourceData> getResource(String resource, bool stream);
		std::unique_ptr<ResourceData> tryGetResource(String resource, bool stream);
		std::unique_ptr<ResourceDataStatic> getStatic(String resource) override;
		std::unique_ptr<ResourceDataStream> getStream(String resource) override;
		std::time_t getTimestamp(String resource);
		StringArray enumerate(String prefix = "", bool removePrefix = false, String suffixMatch = "");

	private:
		HashMap<String, IResourceLocatorProvider*> locators;
		Vector<std::unique_ptr<IResourceLocatorProvider>> locatorList;
	};
}
