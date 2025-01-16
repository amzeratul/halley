#include "remote_profiler_window.h"

using namespace Halley;

RemoteProfilerDisplay::RemoteProfilerDisplay(UIFactory& factory, const HalleyAPI& api)
	: UIWidget("remote_profiler_display", {}, {})
{
	statsView = std::make_shared<PerformanceStatsView>(factory.getResources(), api, false);
	statsView->setDrawBg(false);
	setPage(0);
	setInteractWithMouse(true);
}

void RemoteProfilerDisplay::update(Time t, bool moved)
{
	statsView->update(t);
}

void RemoteProfilerDisplay::draw(UIPainter& painter) const
{
	painter.withClip(getRect()).draw([=](Painter& p)
	{
		statsView->setMousePos(mousePos);
		statsView->paintAt(getRect(), p);
	});
}

void RemoteProfilerDisplay::setProfileData(std::shared_ptr<ProfilerData> profileData)
{
	statsView->onProfileData(std::move(profileData));
}

void RemoteProfilerDisplay::setPage(int page)
{
	statsView->setPage(page + 1);
}

int RemoteProfilerDisplay::getPage() const
{
	return statsView->getPage() - 1;
}

int RemoteProfilerDisplay::getNumPages() const
{
	return statsView->getNumPages() - 1;
}

PerformanceStatsView& RemoteProfilerDisplay::getView() const
{
	return *statsView;
}

void RemoteProfilerDisplay::onMouseOver(Vector2f mousePos)
{
	this->mousePos = mousePos;
}

void RemoteProfilerDisplay::onMouseLeft(Vector2f mousePos)
{
	this->mousePos = {};
}

RemoteProfilerWindow::RemoteProfilerWindow(UIFactory& factory, std::shared_ptr<DevConServerConnection> connection, const HalleyAPI& api)
	: UIWidget("remote_profiler_window", {}, UISizer())
	, factory(factory)
	, connection(std::move(connection))
	, api(api)
{
	factory.loadUI(*this, "halley/remote_profiler_window");
}

void RemoteProfilerWindow::onMakeUI()
{
	display = std::make_shared<RemoteProfilerDisplay>(factory, api);
	auto displayContainer = getWidget("profilerDisplayContainer");
	displayContainer->add(display, 1);

	setHandle(UIEventType::ButtonClicked, "prevPage", [=] (const UIEvent& event)
	{
		display->setPage(modulo(display->getPage() - 1, display->getNumPages()));
	});

	setHandle(UIEventType::ButtonClicked, "nextPage", [=] (const UIEvent& event)
	{
		display->setPage(modulo(display->getPage() + 1, display->getNumPages()));
	});

	setHandle(UIEventType::ButtonClicked, "pause", [=] (const UIEvent& event)
	{
		const bool paused = !display->getView().isPaused();
		display->getView().setPaused(paused);
		getWidgetAs<UIButton>("pause")->setLabel(LocalisedString::fromHardcodedString(paused ? "Resume" : "Pause"));
	});
}

void RemoteProfilerWindow::onActiveChanged(bool active)
{
	setListeningToProfile(active);
}

void RemoteProfilerWindow::update(Time t, bool moved)
{
	display->setProfileData(lastProfileData);
}

void RemoteProfilerWindow::setListeningToProfile(bool listening)
{
	if (listening != isListening) {
		if (activeInterest) {
			connection->getParent().unregisterInterest(*activeInterest);
			activeInterest = {};
		}

		if (listening) {
			activeInterest = connection->getParent().registerInterest("profiler", {}, [=] (size_t idx, ConfigNode result)
			{
				if (result.getType() == ConfigNodeType::Bytes) {
					auto profilerData = Deserializer::fromBytes<ProfilerData>(result.asBytes(), SerializerOptions(SerializerOptions::maxVersion));
					onProfileData(std::make_shared<ProfilerData>(std::move(profilerData)));
				} else {
					lastProfileData = {};
				}
			}, connection->getId());
		}

		isListening = listening;
	}
}

void RemoteProfilerWindow::onProfileData(std::shared_ptr<ProfilerData> data)
{
	lastProfileData = std::move(data);
}
