#include "halley/ui/widgets/ui_render_surface.h"

using namespace Halley;

UIRenderSurface::UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer)
	: UIWidget(std::move(id), minSize, std::move(sizer))
{}

void UIRenderSurface::drawChildren(UIPainter& painter) const
{
	// TODO: draw into private painter
}

void UIRenderSurface::draw(UIPainter& painter) const
{
	// TODO: queue drawing of surface
}

void UIRenderSurface::render(RenderContext& rc) const
{
	// TODO: update surface
}
