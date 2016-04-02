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

namespace Halley {
	class IResourceLocator {
		friend class ResourceLocator;

	public:
		virtual ~IResourceLocator() {}

	protected:
		virtual std::unique_ptr<ResourceData> doGet(String resource, bool stream)=0;
		virtual std::time_t doGetTimestamp(String resource) = 0;
		virtual std::vector<String> getResourceList()=0;
		virtual int getPriority() { return 0; }
	};

	class ResourceLocator
	{
	public:
		void add(std::unique_ptr<IResourceLocator> locator);
		void addFileSystem();
		
		std::unique_ptr<ResourceDataStatic> getStatic(String resource);
		std::unique_ptr<ResourceDataStream> getStream(String resource);
		std::time_t getTimestamp(String resource);
		StringArray enumerate(String prefix = "", bool removePrefix = false, String suffixMatch = "");

	private:
		std::unique_ptr<ResourceData> getResource(String resource, bool stream);
		std::map<String, IResourceLocator*> locators;
		std::vector<std::unique_ptr<IResourceLocator>> locatorList;
	};
}
