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

#include <map>
#include <ctime>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>

namespace Halley {
	class ResourceData;

	class IResourceLocatorProvider {
		friend class ResourceLocator;

	public:
		virtual ~IResourceLocatorProvider() {}

	protected:
		virtual std::unique_ptr<ResourceData> doGet(String resource, bool stream)=0;
		virtual std::time_t doGetTimestamp(String resource) = 0;
		virtual std::vector<String> getResourceList()=0;
		virtual int getPriority() { return 0; }
	};

	class ResourceLocator : public IResourceLocator
	{
	public:
		void add(std::unique_ptr<IResourceLocatorProvider> locator);
		void addFileSystem(String path);
		void addStandardFileSystem();
		
		std::unique_ptr<ResourceData> getResource(String resource, bool stream);
		std::unique_ptr<ResourceData> tryGetResource(String resource, bool stream);
		std::unique_ptr<ResourceDataStatic> getStatic(String resource) override;
		std::unique_ptr<ResourceDataStream> getStream(String resource) override;
		std::time_t getTimestamp(String resource);
		StringArray enumerate(String prefix = "", bool removePrefix = false, String suffixMatch = "");

	private:
		std::map<String, IResourceLocatorProvider*> locators;
		std::vector<std::unique_ptr<IResourceLocatorProvider>> locatorList;
	};
}
