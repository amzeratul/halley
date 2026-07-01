#pragma once

#include <utility>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <halley/concurrency/shared_recursive_mutex.h>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>

#include "halley/support/debug.h"
#include "halley/time/halleytime.h"

// Virtual resource get is necessary for proper editor functionality, but might incur a very small performance penalty
#ifdef DEV_BUILD
#define VIRTUAL_RESOURCE_GET
#endif

namespace Halley
{
	class AsyncResource;
	enum class AssetType;
	class Resource;
	class Resources;
	class ResourceLoader;
	struct ResourceMemoryUsage;

	class ResourceCollectionBase
	{
		class Wrapper
		{
		public:
			Wrapper() = default;
			Wrapper(std::shared_ptr<Resource> resource)
				: res(std::move(resource))
			{}

			std::shared_ptr<Resource> res;
		};

	public:
		using ResourceLoaderFunc = std::function<std::shared_ptr<Resource>(std::string_view, ResourceLoadPriority)>;
		using ResourceEnumeratorFunc = std::function<Vector<String>()>;

		explicit ResourceCollectionBase(Resources& parent, AssetType type);
		virtual ~ResourceCollectionBase() = default;

		void setResourceLoader(ResourceLoaderFunc loader);
		void setResourceEnumerator(ResourceEnumeratorFunc enumerator);

		void clear();
		void unload(std::string_view assetId);
		void unloadAll();
		bool exists(std::string_view assetId) const;
		void setFallback(std::string_view assetId);

		void reload(std::string_view assetId);
		void purge(std::string_view assetId);

		std::shared_ptr<Resource> getUntyped(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal);

		Vector<String> enumerate() const;

		AssetType getAssetType() const;

		ResourceMemoryUsage getMemoryUsage() const;
		void generateDetailedMemoryReport(std::optional<int> limit) const;

		virtual bool isAsync() const = 0;

		void notifyResourcesUnloaded();

		template<typename F>
		void forEachResource(F& f)
		{
			SharedLock lock(mutex);
			for (auto& r: resources) {
				if (r.res) {
					f(r.res);
				}
			}
		}

		template<typename F>
		void forEachResource(const F& f) const
		{
			SharedLock lock(mutex);
			for (auto& r: resources) {
				if (r.res) {
					f(r.res);
				}
			}
		}

		Vector<std::shared_ptr<Resource>> getAllResources() const;

#ifdef VIRTUAL_RESOURCE_GET
		virtual std::shared_ptr<Resource> get(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal, bool allowFallback = true);
#endif

	protected:
		virtual std::shared_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(std::string_view name, ResourceLoadPriority priority, bool allowFallback);
		std::pair<std::shared_ptr<Resource>, bool> loadAsset(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback);

	private:
		Resources& parent;
		HashMap<String, uint32_t> resourceMap;
		Vector<Wrapper> resources;
		String fallback;
		AssetType type;
		ResourceLoaderFunc resourceLoader;
		ResourceEnumeratorFunc resourceEnumerator;
		uint32_t curIdx = 1;

		mutable SharedRecursiveMutex mutex;
		mutable SharedRecursiveMutex loadingMutex;
		mutable ConditionVariableAny resourceLoaded;
		HashSet<String> resourcesLoading;

		Vector<uint32_t> freeIdxs;

		Wrapper& allocateWrapper(const String& id);
	};

	template <typename T>
	class ResourceCollection final : public ResourceCollectionBase
	{
		static_assert(std::is_base_of<Resource, T>::value, "Type must extend Resource");

	public:
		ResourceCollection(Resources& parent, AssetType type)
			: ResourceCollectionBase(parent, type)
		{}

		~ResourceCollection() override
		{
			clear();
		}

#ifndef VIRTUAL_RESOURCE_GET
		std::shared_ptr<const T> get(std::string_view assetId, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(assetId, priority, true));
		}
#endif

	protected:
		std::shared_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}

		bool isAsync() const override
		{
			return std::is_base_of_v<AsyncResource, T>;
		}
	};
}
