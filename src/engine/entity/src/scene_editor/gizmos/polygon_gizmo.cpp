#include "polygon_gizmo.h"
using namespace Halley;

PolygonGizmo::PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options)
	: componentName(componentName)
	, fieldName(fieldName)
	, isOpenPolygon(options["isOpenPolygon"].asBool(false))
{}

void PolygonGizmo::update(Time time, const SceneEditorInputState& inputState)
{}

void PolygonGizmo::draw(Painter& painter) const
{}
