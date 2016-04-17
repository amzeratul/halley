#include "painter.h"
#include "render_context.h"
#include "render_target/render_target.h"
#include "material.h"
#include "material_parameter.h"

using namespace Halley;

void Painter::startRender()
{
	verticesPending = 0;
	bytesPending = 0;
	doStartRender();
}

void Painter::endRender()
{
	flush();
	doEndRender();
}

void Painter::flush()
{
	flushPending();
}

void Painter::drawQuads(std::shared_ptr<Material> material, size_t numVertices, void* vertexData)
{
	assert(numVertices > 0);
	assert(numVertices % 4 == 0);
	assert(vertexData != nullptr);

	if (material != materialPending) {
		if (materialPending != std::shared_ptr<Material>()) {
			flushPending();
		}
		materialPending = material;
	}

	size_t dataSize = numVertices * material->getVertexStride();
	size_t requiredSize = dataSize + bytesPending;
	if (vertexBuffer.size() < requiredSize) {
		vertexBuffer.resize(requiredSize * 2);
	}
	
	memcpy(vertexBuffer.data() + bytesPending, vertexData, dataSize);

	verticesPending += numVertices;
	bytesPending += dataSize;
}

void Painter::bind(RenderContext& context)
{
	// Set render target
	auto& rt = context.getRenderTarget();
	rt.bind();
	
	// Set viewport
	auto viewPort = context.getViewPort();
	setViewPort(viewPort, viewPort != rt.getViewPort());

	// Set camera
	auto& cam = context.getCamera();
	cam.setViewArea(Vector2f(viewPort.getSize()));
	cam.updateProjection();
	projection = cam.getProjection();
}

void Painter::flushPending()
{
	if (verticesPending > 0) {
		executeDrawQuads(*materialPending, verticesPending, vertexBuffer.data());
	}

	// Reset
	bytesPending = 0;
	verticesPending = 0;
	materialPending.reset();
}

void Painter::executeDrawQuads(Material& material, size_t numVertices, void* vertexData)
{
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
