#include "plotter.h"

#include "plotter_canvas.h"

using namespace Halley;

Plotter::Plotter(UIFactory& factory)
	: UIWidget("plotter", {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/plotter");
}

void Plotter::onMakeUI()
{
	auto canvas = std::make_shared<PlotterCanvas>(factory);
	getWidget("canvasContainer")->add(canvas, 1);

	bindData("input", "", [=] (String value)
	{
		canvas->setPlot(value);
	});
}
