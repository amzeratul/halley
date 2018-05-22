#pragma once

#include <functional>
#include "halley/text/halleystring.h"

namespace Halley {
	class UIWidget;
	class String;

	class UIDataBind {
		friend class UIWidget;

    public:
		enum class Format {
			Int,
			Float,
			String
		};

		virtual ~UIDataBind();

		virtual int getIntData();
		virtual float getFloatData();
		virtual String getStringData();

		void pushData();

		virtual Format getFormat() const = 0;

	protected:
		virtual void onDataFromWidget(int data);
		virtual void onDataFromWidget(float data);
		virtual void onDataFromWidget(const String& data);

		bool canWriteData() const;

	private:
		UIWidget* widgetBound = nullptr;
		bool acceptingData = false;

		void setWidget(UIWidget* widget);
		void setAcceptingDataFromWidget(bool accepting);
    };

	class UIDataBindInt : public UIDataBind {
	public:
		using WriteCallback = std::function<void(int)>;

		UIDataBindInt(int initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;

	private:
		int initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindFloat : public UIDataBind {
	public:
		using WriteCallback = std::function<void(float)>;

		UIDataBindFloat(float initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;

	private:
		float initialValue;
		WriteCallback writeCallback;
	};

	class UIDataBindString : public UIDataBind {
	public:
		using WriteCallback = std::function<void(String)>;

		UIDataBindString(String initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;

		Format getFormat() const override;

	protected:
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;

	private:
		String initialValue;
		WriteCallback writeCallback;
	};
}
