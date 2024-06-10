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
	auto p2 = p.withClip(getRect());
	p2.draw(bg);

	if (polygons.empty()) {
		return;
	}

	Rect4f aabb = Rect4f::getSpanningRect(polygons[0]);
	for (size_t i = 1; i < polygons.size(); ++i) {
		aabb = aabb.merge(Rect4f::getSpanningRect(polygons[1]));
	}
	const auto rect = getRect().grow(-10.0f);
	const auto c0 = aabb.getCenter();
	const auto c1 = rect.getCenter();

	const auto zoom = std::min(rect.getHeight() / aabb.getHeight(), rect.getWidth() / aabb.getWidth());
	auto polys = polygons;
	for (auto& poly: polys) {
		for (auto& v: poly) {
			v = (v - c0) * zoom + c1;
		}
	}

	p2.draw([circle = circle, polys = std::move(polys)] (Painter& painter)
	{
		int i = 0;
		for (auto& poly: polys) {
			auto col = Colour4f::fromHSV(static_cast<float>(i++) / 0.41f, 0.5f, 1.0f, 1.0f);
			painter.drawLine(poly, 1.0f, col, true);

			auto c = circle.clone().setColour(col);
			for (size_t i = 0; i < poly.size(); ++i) {
				c.setPosition(poly[i]).draw(painter);
			}
		}
	});
}

void PlotterCanvas::setPlot(std::string_view str)
{
	// Hacky parser :)
	Vector<Vertex> curVertices;

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
							curVertices.push_back(Vertex(number.size() >= 1 ? number[0] : 0, number.size() >= 2 ? number[1] : 0));
							number.clear();
						}
					}
				} else {
					if (c == '(') {
						parsingVertex = true;
					}
					if (c == ']') {
						parsingList = false;
						if (!curVertices.empty()) {
							polygons.push_back(std::move(curVertices));
							curVertices = {};
						}
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
