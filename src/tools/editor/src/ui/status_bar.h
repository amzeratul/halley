#pragma once

namespace Halley {
	class ProjectWindow;

	class StatusBar : public UIWidget, public ILoggerSink
	{
	public:
		StatusBar(UIFactory& factory, ProjectWindow& projectWindow);
		~StatusBar() override;

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	protected:
		void log(LoggerLevel level, std::string_view msg) override;

	private:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		
		Sprite background;
		TextRenderer statusText;

		std::mutex mutex;
		std::optional<std::pair<LoggerLevel, String>> nextStatus;
	};
}