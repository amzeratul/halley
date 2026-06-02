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

		template <typename T>
		[[nodiscard]] ResourceCollectionBase& ofBase() const
		{
			return ofType(T::getAssetType());
		}

		[[nodiscard]] ResourceCollectionBase& ofType(AssetType assetType) const
		{
			return *resources[int(assetType)];
		}

		template <typename T>
		std::shared_ptr<const T> get(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal) const
		{
#ifdef VIRTUAL_RESOURCE_GET
			return std::static_pointer_cast<T>(ofBase<T>().get(name, priority));
#else
			return of<T>().get(name, priority);
#endif
		}

		template <typename T>
		std::shared_ptr<const T> tryGet(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal) const
		{
#ifdef VIRTUAL_RESOURCE_GET
			auto& collection = ofBase<T>();
			if (collection.exists(name)) {
				return std::static_pointer_cast<T>(collection.get(name, priority));
			}
			return {};
#else
			auto& collection = of<T>();
			if (collection.exists(name)) {
				return collection.get(name, priority);
			}
			return {};
#endif
		}

		template <typename T>
		void preload(std::string_view name) const
		{
#ifdef VIRTUAL_RESOURCE_GET
			static_cast<void>(ofBase<T>().get(name, ResourceLoadPriority::Low));
#else
			static_cast<void>(of<T>().get(name, ResourceLoadPriority::Low));
#endif
		}

		template <typename T>
		void preloadAll() const
		{
			for (const auto& e: enumerate<T>()) {
				preload<T>(e);
				std::this_thread::yield();
			}
		}

		template <typename T>
		Vector<Future<void>> preloadAllParallelAsVector(size_t nThreadsReq) const
		{
			Vector<Future<void>> pending;

			if (nThreadsReq == 1) {
				pending += Concurrent::execute([this]
				{
					preloadAll<T>();
				});
			} else {
				const auto entries = enumerate<T>();
				const auto nEntries = entries.size();
				const auto nThreads = std::min(nEntries, nThreadsReq);

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
			}

			return pending;
		}

		template <typename T>
		Future<void> preloadAllParallel(size_t nThreadsReq) const
		{
			return Concurrent::whenAll(preloadAllParallelAsVector<T>(nThreadsReq));
		}

		template <typename T>
		void unload(std::string_view name) const
		{
			ofBase<T>().unload(name);
		}

		template <typename T>
		void unload(const std::shared_ptr<const T>& res)
		{
			ofBase<T>().unload(res->getAssetId());
		}

		template <typename T>
		void setFallback(std::string_view name)
		{
			ofBase<T>().setFallback(name);
		}

		template <typename T>
		[[nodiscard]] bool exists(std::string_view name) const
		{
			return ofBase<T>().exists(name);
		}

		template <typename T>
		[[nodiscard]] Vector<String> enumerate() const
		{
			return ofBase<T>().enumerate();
		}

		ResourceLocator& getLocator()
		{
			return *locator;
		}

		void reloadAssets(const Vector<String>& assetIds, const Vector<String>& packIds); // assetIds are in "type:name" format
		void reloadAssets(const std::map<AssetType, Vector<String>>& byType);

		const ResourceOptions& getOptions() const { return options; }

		void generateMemoryReport();
		void generateDetailedMemoryReport(AssetType type, std::optional<int> limit = {});

	private:
		const std::unique_ptr<ResourceLocator> locator;
		Vector<std::unique_ptr<ResourceCollectionBase>> resources;
		const HalleyAPI* const api;
		ResourceOptions options;
	};
}
