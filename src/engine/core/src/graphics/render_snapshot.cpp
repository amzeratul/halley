#include "halley/core/graphics/render_snapshot.h"
using namespace Halley;

void RenderSnapshot::start()
{
	// TODO
}

void RenderSnapshot::end()
{
	// TODO
}

void RenderSnapshot::clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	// TODO
}

void RenderSnapshot::draw(Material& material, size_t size, gsl::span<char> span, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads)
{
	// TODO
}

void RenderSnapshot::setClip(Rect4i rect, bool enable)
{
	// TODO	
}

void RenderSnapshot::bind(RenderContext& context)
{
	// TODO
}

void RenderSnapshot::unbind(RenderContext& context)
{
	// TODO
}
