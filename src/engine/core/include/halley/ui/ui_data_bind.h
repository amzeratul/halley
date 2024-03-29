#pragma once

#include <functional>

#include "halley/data_structures/config_node.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class UIWidget;
	class String;

	class UIDataBind {
		friend class UIWidget;

    public:
		enum class Format {
			Undefined,
			Bool,
			Int,
			Float,
			String,
			ConfigNode
		};

		virtual ~UIDataBind();

		virtual bool getBoolData();
		virtual int getIntData();
		virtual float getFloatData();
		virtual String getStringData();
		virtual ConfigNode getConfigData();

		void pushData();

		virtual Format getFormat() const = 0;

	protected:
		virtual void onDataFromWidget(bool data);
		virtual void onDataFromWidget(int data);
		virtual void onDataFromWidget(float data);
		virtual void onDataFromWidget(const String& data);
		virtual void onDataFromWidget(const ConfigNode& data);

		bool canWriteData() const;

	private:
		UIWidget* widgetBound = nullptr;
		bool acceptingData = false;

		void setWidget(UIWidget* widget);
		void setAcceptingDataFromWidget(bool accepting);
    };

	class UIDataBindBool final : public UIDataBind {
	public:
		using WriteCallback = std::function<void(bool)>;

		UIDataBindBool(bool initialValue, WriteCallback writeCallback);

		bool getBoolData() override;
		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;
		ConfigNode getConfigData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(bool data) override;
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;
		void onDataFromWidget(const ConfigNode& data) override;

	private:
		int initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindInt final : public UIDataBind {
	public:
		using WriteCallback = std::function<void(int)>;

		UIDataBindInt(int initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;
		ConfigNode getConfigData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(bool data) override;
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;
		void onDataFromWidget(const ConfigNode& data) override;

	private:
		int initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindFloat final : public UIDataBind {
	public:
		using WriteCallback = std::function<void(float)>;

		UIDataBindFloat(float initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;
		ConfigNode getConfigData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(bool data) override;
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;
		void onDataFromWidget(const ConfigNode& data) override;

	private:
		float initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindString final : public UIDataBind {
	public:
		using WriteCallback = std::function<void(String)>;

		UIDataBindString(String initialValue, WriteCallback writeCallback);

		bool getBoolData() override;
		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;
		ConfigNode getConfigData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(bool data) override;
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;
		void onDataFromWidget(const ConfigNode& data) override;

	private:
		String initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindConfigNode final : public UIDataBind {
	public:
		using WriteCallback = std::function<void(ConfigNode)>;

		UIDataBindConfigNode(ConfigNode initialValue, WriteCallback writeCallback);

		bool getBoolData() override;
		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;
		ConfigNode getConfigData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(bool data) override;
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;
		void onDataFromWidget(const ConfigNode& data) override;

	private:
		ConfigNode initialValue;
		WriteCallback writeCallback;
	};
}
