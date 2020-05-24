#pragma once
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/flat_map.h"

namespace Halley {
	class ResourceLoader;
	class ConfigNode;
	class VariableTable;

	namespace Internal {
		struct VariableStorage {
			VariableStorage() noexcept;
			VariableStorage(const VariableStorage& other) noexcept;
			VariableStorage(VariableStorage&& other) noexcept;
			VariableStorage& operator=(const VariableStorage& other) noexcept;
			VariableStorage& operator=(VariableStorage&& other) noexcept;
			
			ConfigNodeType type = ConfigNodeType::Undefined;

			union {
				int intValue;
				float floatValue;
				Vector2i vector2iValue;
				Vector2f vector2fValue;
			};

			void getValue(bool& v) const
			{
				Expects(type == ConfigNodeType::Int);
				v = intValue != 0;
			}

			void getValue(int& v) const
			{
				Expects(type == ConfigNodeType::Int);
				v = intValue;
			}
			
			void getValue(float& v) const
			{
				Expects(type == ConfigNodeType::Int || type == ConfigNodeType::Float);
				if (type == ConfigNodeType::Int) {
					v = float(intValue);
				} else if (type == ConfigNodeType::Float) {
					v = floatValue;
				}				
			}
			
			void getValue(Vector2i& v) const
			{
				Expects(type == ConfigNodeType::Int2);
				v = vector2iValue;
			}
			
			void getValue(Vector2f& v) const
			{
				Expects(type == ConfigNodeType::Float2);
				v = vector2fValue;
			}

			void setValue(const ConfigNode& node);
			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};
		
		class VariableBase {
		public:		
			VariableBase();
			VariableBase(const VariableBase& other) = default;
			VariableBase(VariableBase&& other) noexcept = default;
			VariableBase& operator=(const VariableBase& other) = default;
			VariableBase& operator=(VariableBase&& other) noexcept = default;

		protected:
			VariableBase(const VariableTable& parent, String key);

			const VariableTable* parent = nullptr;
			String key;
			int parentVersion = -1;
			VariableStorage storage;

			void refresh();
		};		
	}

	template <typename T>
	class Variable final : public Internal::VariableBase {
	public:
		Variable() = default;
		
		Variable(const VariableTable& parent, String key)
			: VariableBase(parent, std::move(key))
		{}
		
		operator T()
		{
			T v;
			refresh();
			storage.getValue(v);
			return v;
		}
		
		T get()
		{
			T v;
			refresh();
			storage.getValue(v);
			return v;
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

		const Internal::VariableStorage& getRawStorage(const String& key) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		constexpr static AssetType getAssetType() { return AssetType::VariableTable; }
		static std::unique_ptr<VariableTable> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

	private:
		FlatMap<String, Internal::VariableStorage> variables;
	};
};
