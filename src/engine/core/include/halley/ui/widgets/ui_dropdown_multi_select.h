#pragma once

#include "ui_dropdown.h"

namespace Halley {
	class UIDropdownMultiSelect : public UIDropdown
	{
	public:
		explicit UIDropdownMultiSelect(String id, UIStyle style, Vector<LocalisedString> options = {}, Vector<int> defaultOptions = {});

		void setSelectedOption(int option) override;
		void setSelectedOption(const String& id) override;
		
		void setOptions(Vector<Entry> options, Vector<int> defaultOptions = {});

	protected:
		void readFromDataBind() override;
		void toggleOption(int idx);

		void updateTopLabel() override;
		void updateOptionLabels() override;

		bool canNotifyAsDropdown() const override;
		
	private:
		Vector<int> curOptions;

		void notify();
	};
}
