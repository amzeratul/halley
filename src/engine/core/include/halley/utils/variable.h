#pragma once
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/flat_map.h"
#include "halley/maths/colour.h"

namespace Halley {
	class ResourceLoader;
	class ConfigNode;
	class VariableTable;

	namespace Internal {
		class VariableBase {
		public:		
			VariableBase() = default;
			VariableBase(const VariableBase& other) = default;
			VariableBase(VariableBase&& other) noexcept = default;
			VariableBase& operator=(const VariableBase& other) = default;
			VariableBase& operator=(VariableBase&& other) noexcept = default;

		protected:
			VariableBase(const VariableTable& parent, String key);

			const VariableTable* parent = nullptr;
			String key;
			mutable int parentVersion = -1;
			mutable ConfigNode storage;

			void refresh() const;
		};		
	}

	template <typename T>
	class Variable final : public Internal::VariableBase {
	public:
		Variable() = default;
		
		Variable(const VariableTable& parent, String key)
			: VariableBase(parent, std::move(key))
		{}

		Variable(const Variable<T>& other) = delete;
		Variable(Variable<T>&& other) = default;
		Variable& operator=(const Variable<T>& other) = delete;
		Variable& operator=(Variable<T>&& other) = default;

		bool isValid() const
		{
			return parent != nullptr;
		}

		operator T() const
		{
			refresh();
			return storage.asType<T>();
		}
		
		T get() const
		{
			refresh();
			return storage.asType<T>();
		}
	};

	class VariableTable final : public Resource {
	public:
		VariableTable();
		VariableTable(const ConfigNode& node);

		template <typename T>
		Variable<T> get(String name) const
		{
			return Variable<T>(*this, std::move(name));
		}

		const ConfigNode& getRawStorage(const String& key) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		constexpr static AssetType getAssetType() { return AssetType::VariableTable; }
		static std::unique_ptr<VariableTable> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

	private:
		FlatMap<String, ConfigNode> variables;
		ConfigNode dummy;
	};
};
