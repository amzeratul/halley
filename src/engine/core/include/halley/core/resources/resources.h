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
		struct Options {
			bool retainPixelData = false;

			Options() = default;
			Options(bool retainPixelData)
				: retainPixelData(retainPixelData)
			{}
		};
		
		Resources(std::unique_ptr<ResourceLocator> locator, const HalleyAPI& api, Options options);
		~Resources();

		template <typename T>
		void init()
		{
			constexpr AssetType assetType = T::getAssetType();
			constexpr int id = int(assetType);
			resources.resize(std::max(resources.size(), size_t(id + 1)));
			resources[id] = std::make_unique<ResourceCollection<T>>(*this, assetType);
		}

		template <typename T>
		[[nodiscard]] ResourceCollection<T>& of() const
		{
			return static_cast<ResourceCollection<T>&>(ofType(T::getAssetType()));
		}

		[[nodiscard]] ResourceCollectionBase& ofType(AssetType assetType) const
		{
			return *resources[int(assetType)];
		}

		template <typename T>
		std::shared_ptr<const T> get(const String& name, ResourceLoadPriority priority = ResourceLoadPriority::Normal) const
		{
			return of<T>().get(name, priority);
		}

		template <typename T>
		void unload(const String& name) const
		{
			of<T>().unload(name);
		}

		template <typename T>
		void unload(const std::shared_ptr<const T>& res)
		{
			of<T>().unload(res->getAssetId());
		}

		template <typename T>
		void setFallback(const String& name)
		{
			of<T>().setFallback(name);
		}

		template <typename T>
		[[nodiscard]] bool exists(const String& name) const
		{
			return of<T>().exists(name);
		}

		template <typename T>
		[[nodiscard]] std::vector<String> enumerate() const
		{
			return of<T>().enumerate();
		}

		ResourceLocator& getLocator()
		{
			return *locator;
		}

		void reloadAssets(const std::vector<String>& ids); // ids are in "type:name" format
		void reloadAssets(const std::map<AssetType, std::vector<String>>& byType);

		const Options& getOptions() const { return options; }

	private:
		const std::unique_ptr<ResourceLocator> locator;
		Vector<std::unique_ptr<ResourceCollectionBase>> resources;
		const HalleyAPI* const api;
		Options options;
	};
}
