#pragma once

#include "component_field_parameters.h"
#include "halley/game/scene_editor_interface.h"
#include "halley/ui/ui_widget.h"
#include "halley/data_structures/config_database.h"

namespace Halley {
	class ComponentDataRetriever;

	class BaseEnumFieldFactory : public IComponentEditorFieldFactory
	{
	public:
		std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override;

	protected:
		virtual Vector<String> getValues(const ComponentDataRetriever& data) const = 0;
		virtual String getDefaultValue(const Vector<String>& values, const String& requestedValue) const;
		virtual std::optional<String> getDependentField() const;
	};

	class DependencyObserver : public UIWidget {
	public:
		DependencyObserver(ComponentDataRetriever data, String fieldName, String value, std::function<void(const ComponentDataRetriever&)> refreshCallback);

		void update(Time t, bool moved) override;

	private:
		ComponentDataRetriever data;
		String fieldName;
		String value;
		std::function<void(const ComponentDataRetriever&)> refreshCallback;
	};

	class EnumFieldFactory : public BaseEnumFieldFactory
	{
	public:
		EnumFieldFactory(String name, Vector<String> values, String defaultValue);
		
		String getFieldType() override;

		template <typename T>
		static std::unique_ptr<EnumFieldFactory> makeEnumFactory(String name)
		{
			const auto& vals = EnumNames<T>()();
			auto values = Vector<String>(vals.begin(), vals.end());
			auto defaultValue = values.empty() ? "" : values.front();
			return std::make_unique<EnumFieldFactory>(std::move(name), std::move(values), defaultValue);
		}

		template <typename T>
		static std::unique_ptr<EnumFieldFactory> makeEnumFactory(String name, T defaultValue)
		{
			const auto& vals = EnumNames<T>()();
			return std::make_unique<EnumFieldFactory>(std::move(name), Vector<String>(vals.begin(), vals.end()), toString(defaultValue));
		}

	protected:
		Vector<String> getValues(const ComponentDataRetriever& data) const override;
		String getDefaultValue(const Vector<String>& values, const String& requestedValue) const override;

	private:
		const String fieldName;
		String defaultValue;
		Vector<String> values;
	};

	class EnumIntFieldFactory : public IComponentEditorFieldFactory
	{
	public:
		EnumIntFieldFactory(String name, Vector<String> names, Vector<int> values);
		
		std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override;
		String getFieldType() override;

		template <typename T>
		static std::unique_ptr<EnumIntFieldFactory> makeEnumFactory(String name)
		{
			const auto names = EnumNames<T>();
			const auto& vals = names();
			auto stringValues = Vector<String>(vals.begin(), vals.end());
			Vector<int> intValues;
			intValues.resize(stringValues.size());
			for (size_t i = 0; i < intValues.size(); ++i) {
				intValues[i] = names.getValue(i);
			}
			return std::make_unique<EnumIntFieldFactory>(std::move(name), std::move(stringValues), std::move(intValues));
		}

	private:
		const String fieldName;
		Vector<String> names;
		Vector<int> values;
	};
	
	class ConfigDBFieldFactory : public BaseEnumFieldFactory
	{
	public:
		ConfigDBFieldFactory(String name, Vector<String> values);
		
		String getFieldType() override;

		template <typename T>
		static std::unique_ptr<EnumFieldFactory> makeConfigFactory(String name, const ConfigDatabase& config)
		{
			return std::make_unique<EnumFieldFactory>(std::move(name), config.getKeys<T>());
		}

		template <typename T, typename U>
		static std::unique_ptr<EnumFieldFactory> makeConfigFactory(String name, const ConfigDatabase& config, const String& altPrefix = "!")
		{
			Vector<String> result = config.getKeys<T>();
			for (const auto& k: config.getKeys<U>()) {
				result.push_back(altPrefix + k);
			}
			return std::make_unique<EnumFieldFactory>(std::move(name), result, result.empty() ? result.front() : "");
		}

	protected:
		Vector<String> getValues(const ComponentDataRetriever& data) const override;

	private:
		const String fieldName;
		Vector<String> values;
	};

}
