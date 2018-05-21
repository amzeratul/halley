#pragma once

#include <functional>

namespace Halley {
	class UIWidget;
	class String;

	class UIDataBind {
		friend class UIWidget;

    public:
		virtual ~UIDataBind();

		virtual int getIntData();
		virtual float getFloatData();
		virtual String getStringData();

		void pushData();

	protected:
		virtual void onDataFromWidget(int data);
		virtual void onDataFromWidget(float data);
		virtual void onDataFromWidget(const String& data);

	private:
		UIWidget* widgetBound = nullptr;

		void setWidget(UIWidget* widget);
    };

	class UIDataBindInt : public UIDataBind {
	public:
		using WriteCallback = std::function<void(int)>;

		UIDataBindInt(int initialValue, WriteCallback writeCallback);

		int getIntData() override;
		float getFloatData() override;
		String getStringData() override;

	protected:
		void onDataFromWidget(int data) override;
		void onDataFromWidget(float data) override;
		void onDataFromWidget(const String& data) override;

	private:
		int initialValue;
		WriteCallback writeCallback;
	};
}
