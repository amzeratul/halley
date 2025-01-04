#include "remote_profiler_window.h"

using namespace Halley;

RemoteProfilerDisplay::RemoteProfilerDisplay(UIFactory& factory, const HalleyAPI& api)
	: UIWidget("remote_profiler_display", {}, {})
{
	statsView = std::make_shared<PerformanceStatsView>(factory.getResources(), api, false);
	statsView->setPage(2);
	statsView->setDrawBg(false);
}

void RemoteProfilerDisplay::update(Time t, bool moved)
{
	statsView->update(t);
}

void RemoteProfilerDisplay::draw(UIPainter& painter) const
{
	painter.withClip(getRect()).draw([=](Painter& p)
	{
		statsView->paintAt(getRect(), p);
	});
}

void RemoteProfilerDisplay::setProfileData(std::shared_ptr<ProfilerData> profileData)
{
	statsView->onProfileData(std::move(profileData));
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
				onProfileData(std::make_shared<ProfilerData>(Deserializer::fromBytes<ProfilerData>(result.asBytes(), SerializerOptions(SerializerOptions::maxVersion))));
			}, connection->getId());
		}

		isListening = listening;
	}
}

void RemoteProfilerWindow::onProfileData(std::shared_ptr<ProfilerData> data)
{
	lastProfileData = data;
}
