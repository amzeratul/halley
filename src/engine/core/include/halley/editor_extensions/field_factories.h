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
		EnumFieldFactory(String name, Vector<String> values);
		
		String getFieldType() override;

		template <typename T>
		static std::unique_ptr<EnumFieldFactory> makeEnumFactory(String name)
		{
			const auto& vals = EnumNames<T>()();
			return std::make_unique<EnumFieldFactory>(std::move(name), Vector<String>(vals.begin(), vals.end()));
		}

	protected:
		Vector<String> getValues(const ComponentDataRetriever& data) const override;

	private:
		const String fieldName;
		Vector<String> values;
	};

	class EnumIntFieldFactory : public IComponentEditorFieldFactory
	{
	public:
		EnumIntFieldFactory(String name, Vector<String> names);
		
		std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override;
		String getFieldType() override;

		template <typename T>
		static std::unique_ptr<EnumIntFieldFactory> makeEnumFactory(String name)
		{
			const auto& vals = EnumNames<T>()();
			return std::make_unique<EnumIntFieldFactory>(std::move(name), Vector<String>(vals.begin(), vals.end()));
		}

	private:
		const String fieldName;
		Vector<String> names;
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

	protected:
		Vector<String> getValues(const ComponentDataRetriever& data) const override;

	private:
		const String fieldName;
		Vector<String> values;
	};

}
