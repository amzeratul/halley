#include "plotter_canvas.h"

using namespace Halley;

PlotterCanvas::PlotterCanvas(UIFactory& factory)
	: UIWidget("plotterCanvas", {})
	, factory(factory)
{
	bg.setImage(factory.getResources(), "whitebox.png").setColour(factory.getColourScheme()->getColour("ui_backgroundBox"));
	circle.setImage(factory.getResources(), "whitebox.png", "Halley/Circle").setPivot(Vector2f(0.5f, 0.5f)).scaleTo(Vector2f(5, 5));
}

void PlotterCanvas::update(Time t, bool moved)
{
	bg.setPosition(getPosition()).scaleTo(getRect().getSize());
}

void PlotterCanvas::draw(UIPainter& p) const
{
	p.draw(bg);

	if (vertices.empty()) {
		return;
	}
	const auto aabb = Rect4f::getSpanningRect(vertices).grow(1.0f);
	const auto rect = getRect().grow(-10.0f);
	const auto c0 = aabb.getCenter();
	const auto c1 = rect.getCenter();

	const auto zoom = std::min(rect.getHeight() / aabb.getHeight(), rect.getWidth() / aabb.getWidth());
	auto vs = vertices;
	for (auto& v: vs) {
		v = (v - c0) * zoom + c1;
	}

	p.draw([circle = circle, vs = vs] (Painter& painter)
	{
		painter.drawLine(vs, 1.0f, Colour4f(1, 1, 1), true);

		auto c = circle;
		for (const auto& v: vs) {
			c.setPosition(v).draw(painter);
		}
	});
}

void PlotterCanvas::setPlot(std::string_view str)
{
	// Hacky parser :)
	vertices.clear();

	bool parsingList = false;
	bool parsingVertex = false;

	Vector<float> number;
	String curNumber;

	try {
		for (auto c: str) {
			const bool isNumberDigit = (c >= '0' && c <= '9') || c == '-' || c == '.';

			if (parsingList) {
				if (parsingVertex) {
					if (isNumberDigit) {
						curNumber += c;
					} else {
						if (!curNumber.isEmpty()) {
							number.push_back(curNumber.toFloat());
							curNumber = "";
						}

						if (c == ')') {
							parsingVertex = false;
							vertices.push_back(Vertex(number.size() >= 1 ? number[0] : 0, number.size() >= 2 ? number[1] : 0));
							number.clear();
						}
					}
				} else {
					if (c == '(') {
						parsingVertex = true;
					}
					if (c == ']') {
						parsingList = false;
						return;
					}
				}
			} else {
				if (c == '[') {
					parsingList = true;
				}
			}
		}
	} catch (...) {}
}
