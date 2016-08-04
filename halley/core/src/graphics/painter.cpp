#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <cstring> // memmove

using namespace Halley;

void Painter::startRender()
{
	Material::resetBindCache();
	nDrawCalls = nTriangles = nVertices = 0;

	verticesPending = 0;
	bytesPending = 0;
	doStartRender();
}

void Painter::endRender()
{
	flush();
	doEndRender();
	camera = nullptr;
	viewPort = Rect4i(0, 0, 0, 0);
}

void Painter::flush()
{
	flushPending();
}

/*
template <typename T>
static void doMemcpyAlign(void* dstBytes, const void* srcBytes, size_t bytes)
{
	T* dst = reinterpret_cast<T*>(dstBytes);
	const T* src = reinterpret_cast<const T*>(srcBytes);
	const T* srcEnd = src + (bytes / sizeof(T));
	for (; src != srcEnd; ++src, ++dst) {
		*dst = *src;
	}
}

inline void memcpyAlign(void* dstBytes, const void* srcBytes, size_t bytes)
{
	if (bytes % sizeof(size_t) == 0) {
		// Use size_t aligned if possible
		doMemcpyAlign<size_t>(dstBytes, srcBytes, bytes);
	} else {
		// Data is always int-aligned, since numVertices % 4 == 0
		doMemcpyAlign<int>(dstBytes, srcBytes, bytes);
	}
}
*/

void Painter::drawQuads(std::shared_ptr<Material> material, size_t numVertices, void* vertexData)
{
	assert(numVertices > 0);
	assert(numVertices % 4 == 0);
	assert(vertexData != nullptr);
	assert(material);

	if (material != materialPending) {
		if (materialPending != std::shared_ptr<Material>()) {
			flushPending();
		}
		materialPending = material;
	}

	size_t dataSize = numVertices * material->getDefinition().getVertexStride();
	size_t requiredSize = dataSize + bytesPending;
	if (vertexBuffer.size() < requiredSize) {
		vertexBuffer.resize(requiredSize * 2);
	}
	
	memmove(vertexBuffer.data() + bytesPending, vertexData, dataSize);

	verticesPending += numVertices;
	bytesPending += dataSize;
}

void Painter::bind(RenderContext& context)
{
	// Set render target
	auto& rt = context.getRenderTarget();
	rt.bind();
	
	// Set viewport
	viewPort = context.getViewPort();
	setViewPort(viewPort, viewPort != rt.getViewPort());

	// Set camera
	camera = &context.getCamera();
	camera->setViewArea(Vector2f(viewPort.getSize()));
	camera->updateProjection();
	projection = camera->getProjection();
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
	setVertices(material.getDefinition(), numVertices, vertexData);

	// Go through each pass
	for (int i = 0; i < material.getDefinition().getNumPasses(); i++) {
		// Bind pass
		material.bind(i, *this);

		// Draw
		drawQuads(int(numVertices / 4));

		// Log stats
		nDrawCalls++;
		nTriangles += numVertices / 2;
		nVertices += numVertices;
	}
}
