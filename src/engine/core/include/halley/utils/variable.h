#pragma once
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/flat_map.h"
#include "halley/maths/colour.h"

namespace Halley {
	class ResourceLoader;
	class ConfigNode;
	class VariableTable;

	namespace Internal {
		enum class VariableType {
			Undefined,
			Bool,
			Int,
			Float,
			Int2,
			Float2,
			String,
			Colour
		};
		
		struct VariableStorage {
			VariableStorage() noexcept;
			VariableStorage(const VariableStorage& other) noexcept;
			VariableStorage(VariableStorage&& other) noexcept;
			VariableStorage& operator=(const VariableStorage& other) noexcept;
			VariableStorage& operator=(VariableStorage&& other) noexcept;
			~VariableStorage();
			
			VariableType type = VariableType::Undefined;

			union {
				bool boolValue;
				int intValue;
				float floatValue;
				Vector2i vector2iValue;
				Vector2f vector2fValue;
				Colour4f colourValue;
				String* stringValue;
			};

			void getValue(bool& v) const
			{
				Expects(type == VariableType::Bool);
				v = boolValue;
			}

			void getValue(int& v) const
			{
				Expects(type == VariableType::Int);
				v = intValue;
			}
			
			void getValue(float& v) const
			{
				Expects(type == VariableType::Int || type == VariableType::Float);
				if (type == VariableType::Int) {
					v = float(intValue);
				} else if (type == VariableType::Float) {
					v = floatValue;
				}				
			}
			
			void getValue(Vector2i& v) const
			{
				Expects(type == VariableType::Int2);
				v = vector2iValue;
			}
			
			void getValue(Vector2f& v) const
			{
				Expects(type == VariableType::Float2);
				v = vector2fValue;
			}
			
			void getValue(Colour4f& v) const
			{
				Expects(type == VariableType::Colour);
				v = colourValue;
			}
			
			void getValue(String& v) const
			{
				Expects(type == VariableType::String);
				v = *stringValue;
			}

			void setValue(const ConfigNode& node);
			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

		private:
			void clear();
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
			mutable int parentVersion = -1;
			mutable VariableStorage storage;

			void refresh() const;
		};		
	}

	template <>
	struct EnumNames<Internal::VariableType> {
		constexpr std::array<const char*, 8> operator()() const {
			return{{
				"undefined",
				"bool",
				"int",
				"float",
				"int2",
				"float2",
				"string",
				"colour"
			}};
		}
	};

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

		operator T() const
		{
			T v;
			refresh();
			storage.getValue(v);
			return v;
		}
		
		T get() const
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
		Internal::VariableStorage dummy;
	};
};
