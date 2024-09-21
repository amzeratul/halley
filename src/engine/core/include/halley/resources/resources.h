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
#include <halley/concurrency/concurrent.h>
#include <halley/support/exception.h>
#include "halley/resources/resource.h"
#include "resource_collection.h"
#include "halley/text/enum_names.h"

namespace Halley {
	
	class ResourceLocator;
	class HalleyAPI;
	
	class Resources {
		friend class ResourceCollectionBase;

	public:
		Resources(std::unique_ptr<ResourceLocator> locator, const HalleyAPI& api, ResourceOptions options);
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
		std::shared_ptr<const T> get(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal) const
		{
			return of<T>().get(name, priority);
		}

		template <typename T>
		void preload(std::string_view name) const
		{
			static_cast<void>(of<T>().get(name, ResourceLoadPriority::Low));
		}

		template <typename T>
		void preloadAll() const
		{
			for (const auto& e: enumerate<T>()) {
				preload<T>(e);
			}
		}

		template <typename T>
		Vector<Future<void>> preloadAllParallel(size_t nThreadsReq) const
		{
			const auto entries = enumerate<T>();
			const auto nEntries = entries.size();
			const auto nThreads = std::min(nEntries, nThreadsReq);
			Vector<Future<void>> pending;

			for (size_t i = 0; i < nThreads; ++i) {
				auto e0 = (nEntries * i) / nThreads;
				auto e1 = (nEntries * (i + 1)) / nThreads;

				Vector<String> es;
				es.reserve(e1 - e0);
				for (size_t e = e0; e < e1; ++e) {
					es.push_back(std::move(entries[e]));
				}

				pending += Concurrent::execute([this, es = std::move(es)]
				{
					for (auto& e: es) {
						preload<T>(e);
					}
				});
			}

			return pending;
		}

		template <typename T>
		void unload(std::string_view name) const
		{
			of<T>().unload(name);
		}

		template <typename T>
		void unload(const std::shared_ptr<const T>& res)
		{
			of<T>().unload(res->getAssetId());
		}

		template <typename T>
		void setFallback(std::string_view name)
		{
			of<T>().setFallback(name);
		}

		template <typename T>
		[[nodiscard]] bool exists(std::string_view name) const
		{
			return of<T>().exists(name);
		}

		template <typename T>
		[[nodiscard]] Vector<String> enumerate() const
		{
			return of<T>().enumerate();
		}

		ResourceLocator& getLocator()
		{
			return *locator;
		}

		void reloadAssets(const Vector<String>& assetIds, const Vector<String>& packIds); // assetIds are in "type:name" format
		void reloadAssets(const std::map<AssetType, Vector<String>>& byType);

		const ResourceOptions& getOptions() const { return options; }

		void generateMemoryReport();

	private:
		const std::unique_ptr<ResourceLocator> locator;
		Vector<std::unique_ptr<ResourceCollectionBase>> resources;
		const HalleyAPI* const api;
		ResourceOptions options;
	};
}
