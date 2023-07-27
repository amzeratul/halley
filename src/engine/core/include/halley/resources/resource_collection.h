#pragma once

#include <utility>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <halley/concurrency/shared_recursive_mutex.h>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>

namespace Halley
{
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
			Wrapper(std::shared_ptr<Resource> resource, int loadDepth)
				: res(std::move(resource))
				, depth(loadDepth)
			{}

			std::shared_ptr<Resource> res;
			int depth;
		};

	public:
		using ResourceLoaderFunc = std::function<std::shared_ptr<Resource>(std::string_view, ResourceLoadPriority)>;
		using ResourceEnumeratorFunc = std::function<Vector<String>()>;

		explicit ResourceCollectionBase(Resources& parent, AssetType type);
		virtual ~ResourceCollectionBase() {}

		void setResource(int curDepth, std::string_view assetId, std::shared_ptr<Resource> resource);
		void setResourceLoader(ResourceLoaderFunc loader);
		void setResourceEnumerator(ResourceEnumeratorFunc enumerator);

		void clear();
		void unload(std::string_view assetId);
		void unloadAll(int minDepth = 0);
		bool exists(std::string_view assetId) const;
		void setFallback(std::string_view assetId);

		void reload(std::string_view assetId);
		void purge(std::string_view assetId);

		std::shared_ptr<Resource> getUntyped(std::string_view name, ResourceLoadPriority priority = ResourceLoadPriority::Normal);

		Vector<String> enumerate() const;

		AssetType getAssetType() const;

		ResourceMemoryUsage getMemoryUsage() const;
		ResourceMemoryUsage getMemoryUsageAndAge(float time);
		void age(float time);

		/// <returns>How much memory was freed</returns>
		ResourceMemoryUsage clearOldResources(float maxAge);
		void notifyResourcesUnloaded();

	protected:
		virtual std::shared_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(std::string_view name, ResourceLoadPriority priority, bool allowFallback);
		std::pair<std::shared_ptr<Resource>, bool> loadAsset(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback);

	private:
		Resources& parent;
		HashMap<String, Wrapper> resources;
		String fallback;
		AssetType type;
		ResourceLoaderFunc resourceLoader;
		ResourceEnumeratorFunc resourceEnumerator;
		mutable SharedRecursiveMutex mutex;
	};

	template <typename T>
	class ResourceCollection final : public ResourceCollectionBase
	{
		static_assert(std::is_base_of<Resource, T>::value, "Type must extend Resource");

	public:
		ResourceCollection(Resources& parent, AssetType type)
			: ResourceCollectionBase(parent, type)
		{}

		std::shared_ptr<const T> get(std::string_view assetId, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(assetId, priority, true));
		}

	protected:
		std::shared_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}
	};
}
