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
#include <algorithm>
#include <halley/support/exception.h>
#include "halley/resources/resource.h"
#include "resource_collection.h"
#include "halley/text/string_converter.h"

namespace Halley {
	
	class ResourceLocator;
	class HalleyAPI;
	
	class Resources {
		friend class ResourceCollectionBase;

	public:
		Resources(std::unique_ptr<ResourceLocator> locator, HalleyAPI* api);
		~Resources();

		template <typename T>
		void init()
		{
			AssetType assetType = T::getAssetType();
			int id = int(assetType);
			resources.resize(std::max(resources.size(), size_t(id + 1)));
			resources[id] = std::make_unique<ResourceCollection<T>>(*this, assetType);
		}

		template <typename T>
		ResourceCollection<T>& of()
		{
			AssetType assetType = T::getAssetType();
			return static_cast<ResourceCollection<T>&>(*resources[int(assetType)]);
		}

		template <typename T>
		std::shared_ptr<const T> get(const String& name, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return of<T>().get(name, priority);
		}

		template <typename T>
		bool exists(const String& name)
		{
			return of<T>().exists(name);
		}

		template <typename T>
		std::vector<String> enumerate()
		{
			return of<T>().enumerate();
		}
		
	private:
		std::unique_ptr<ResourceLocator> locator;
		Vector<std::unique_ptr<ResourceCollectionBase>> resources;
		HalleyAPI* api;
	};
}
