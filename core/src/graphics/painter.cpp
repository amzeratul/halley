#include "painter.h"
#include "render_context.h"
#include "render_target/render_target.h"
#include "material.h"
#include "material_parameter.h"

using namespace Halley;

void Painter::drawQuads(Material& material, size_t numVertices, void* vertexData)
{
	assert(numVertices % 4 == 0);

	// Bind projection
	material["u_mvp"] = projection;

	// Load vertices
	setVertices(material, numVertices, vertexData);

	// Go through each pass
	for (int i = 0; i < material.getNumPasses(); i++) {
		// Bind pass
		material.bind(i, *this);

		// Draw
		drawQuads(int(numVertices / 4));
	}
}

void Painter::bind(RenderContext& context)
{
	context.getRenderTarget().bind();
	auto& cam = context.getCamera();
	cam.updateProjection();
	projection = cam.getProjection();
}
