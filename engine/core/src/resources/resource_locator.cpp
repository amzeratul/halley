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

#include "resources/resource_locator.h"
#include "resources/resource_filesystem.h"
#include <iostream>
#include <set>
#include <halley/support/exception.h>

namespace Halley {
	ResourceLocator::ResourceLocator(SystemAPI& system)
		: system(system)
	{
	}

	void ResourceLocator::add(std::unique_ptr<IResourceLocatorProvider> locator)
	{
		auto res = locator->getResourceList();
		for (size_t i=0; i<res.size(); i++) {
			// Check if resource is already listed
			auto result = locators.find(res[i]);
			if (result != locators.end()) {
				// Already listed, check if we should override
				if (result->second->getPriority() >= locator->getPriority()) break;
			}
				
			locators[res[i]] = &*locator;
		}
		locatorList.emplace_back(std::move(locator));
	}
	
	std::unique_ptr<ResourceData> ResourceLocator::getResource(String resource, bool stream)
	{
		auto result = tryGetResource(resource, stream);
		if (result) {
			return std::move(result);
		} else {
			// Not found
			throw Exception("Unable to locate resource: " + resource);
		}
	}

	std::unique_ptr<ResourceData> ResourceLocator::tryGetResource(String resource, bool stream)
	{
		auto result = locators.find(resource);
		if (result != locators.end()) {
			// Found a locator
			return result->second->doGet(resource, stream);
		} else {
			// Could not find, try the generic locator
			result = locators.find("*");
			if (result != locators.end()) {
				// Generic locator found, use it
				return result->second->doGet(resource, stream);
			} else {
				// No generic locator, give up
				std::cout << "Resource Locator giving up on resource: " << resource << std::endl;
				return std::unique_ptr<ResourceData>();
			}
		}
	}

	std::unique_ptr<ResourceDataStatic> ResourceLocator::getStatic(String resource)
	{
		auto ptr = dynamic_cast<ResourceDataStatic*>(getResource(resource, false).release());
		if (!ptr) {
			throw Exception("Resource " + resource + " obtained, but is not static data. Memory leak has ocurred.");
		}
		return std::unique_ptr<ResourceDataStatic>(ptr);
	}

	std::unique_ptr<ResourceDataStream> ResourceLocator::getStream(String resource)
	{
		auto ptr = dynamic_cast<ResourceDataStream*>(getResource(resource, true).release());
		if (!ptr) {
			throw Exception("Resource " + resource + " obtained, but is not stream data. Memory leak has ocurred.");
		}
		return std::unique_ptr<ResourceDataStream>(ptr);
	}
	
	StringArray ResourceLocator::enumerate(String prefix, bool removePrefix, String suffixMatch)
	{
		std::set<String> result;
		size_t sz = locatorList.size();
		for (size_t i=0; i<sz; i++) {
			StringArray tmp = locatorList[i]->getResourceList();
			size_t sz2 = tmp.size();
			for (size_t j=0; j<sz2; j++) {
				String str = tmp[j];
				if (str != "*" && str.startsWith(prefix) && str.endsWith(suffixMatch)) {
					if (removePrefix) result.insert(str.substr(prefix.length()));
					else result.insert(str);
				}
			}
		}

		StringArray result2;
		for (auto iter=result.begin(); iter != result.end(); ++iter) {
			result2.push_back(*iter);
		}
		return result2;
	}

	void ResourceLocator::addFileSystem(Path path)
	{
		add(std::make_unique<FileSystemResourceLocator>(system, path));
	}
}
