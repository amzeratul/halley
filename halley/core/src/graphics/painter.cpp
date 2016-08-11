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

	resetPending();
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

static Vector4f& getVertPos(char* vertexAttrib, size_t vertPosOffset)
{
	return *reinterpret_cast<Vector4f*>(vertexAttrib + vertPosOffset);
}

void Painter::drawSprites(std::shared_ptr<Material> material, size_t numSprites, size_t vertPosOffset, const void* vertexData)
{
	assert(numSprites > 0);
	assert(vertexData != nullptr);
	assert(material);

	startDrawCall(material);

	const size_t vertexStride = material->getDefinition().getVertexStride();
	const size_t verticesPerSprite = 4;
	const size_t numVertices = verticesPerSprite * numSprites;
	const size_t dataSize = numVertices * vertexStride;
	makeSpaceForPendingVertices(dataSize);

	char* const dst = vertexBuffer.data() + bytesPending;
	const char* const src = reinterpret_cast<const char*>(vertexData);

	for (size_t i = 0; i < numSprites; i++) {
		for (size_t j = 0; j < verticesPerSprite; j++) {
			size_t srcOffset = i * vertexStride;
			size_t dstOffset = (i * verticesPerSprite + j) * vertexStride;
			memmove(dst + dstOffset, src + srcOffset, vertexStride);

			// j -> vertPos
			// 0 -> 0, 0
			// 1 -> 1, 0
			// 2 -> 1, 1
			// 3 -> 0, 1
			const float x = ((j & 1) ^ ((j & 2) >> 1)) * 1.0f;
			const float y = ((j & 2) >> 1) * 1.0f;
			getVertPos(dst + dstOffset, vertPosOffset) = Vector4f(x, y, x, y);
		}
	}

	generateQuadIndices(verticesPending, numSprites);
	verticesPending += numVertices;
	bytesPending += dataSize;
}

void Painter::drawQuads(std::shared_ptr<Material> material, size_t numVertices, const void* vertexData)
{
	assert(numVertices > 0);
	assert(numVertices % 4 == 0);
	assert(vertexData != nullptr);
	assert(material);

	startDrawCall(material);

	size_t dataSize = numVertices * material->getDefinition().getVertexStride();
	makeSpaceForPendingVertices(dataSize);
	
	memmove(vertexBuffer.data() + bytesPending, vertexData, dataSize);

	generateQuadIndices(verticesPending, numVertices / 4);
	verticesPending += numVertices;
	bytesPending += dataSize;
}

void Painter::makeSpaceForPendingVertices(size_t numBytes)
{
	size_t requiredSize = bytesPending + numBytes;
	if (vertexBuffer.size() < requiredSize) {
		vertexBuffer.resize(requiredSize * 2);
	}
}

void Painter::makeSpaceForPendingIndices(size_t numIndices)
{
	size_t requiredSize = indicesPending + numIndices;
	if (indexBuffer.size() < requiredSize) {
		indexBuffer.resize(requiredSize * 2);
	}
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

void Painter::startDrawCall(std::shared_ptr<Material>& material)
{
	if (material != materialPending) {
		if (materialPending != std::shared_ptr<Material>()) {
			flushPending();
		}
		materialPending = material;
	}
}

void Painter::flushPending()
{
	if (verticesPending > 0) {
		executeDrawTriangles(*materialPending, verticesPending, vertexBuffer.data(), indicesPending, indexBuffer.data());
	}

	resetPending();
}

void Painter::resetPending()
{
	bytesPending = 0;
	verticesPending = 0;
	indicesPending = 0;
	materialPending.reset();
}

void Painter::executeDrawTriangles(Material& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices)
{
	// Bind projection
	material["u_mvp"] = projection;

	// Load vertices
	setVertices(material.getDefinition(), numVertices, vertexData, numIndices, indices);

	// Go through each pass
	for (int i = 0; i < material.getDefinition().getNumPasses(); i++) {
		// Bind pass
		material.bind(i, *this);

		// Draw
		drawTriangles(numIndices);

		// Log stats
		nDrawCalls++;
		nTriangles += numIndices / 3;
		nVertices += numVertices;
	}
}

unsigned short* Painter::getStandardQuadIndices(size_t numQuads)
{
	size_t sz = numQuads * 6;
	size_t oldSize = stdQuadIndexCache.size();

	if (oldSize < sz) {
		stdQuadIndexCache.resize(sz);
		unsigned short pos = static_cast<unsigned short>(oldSize * 2 / 3);
		for (size_t i = oldSize; i < sz; i += 6) {
			// B-----C
			// |     |
			// A-----D
			// ABC
			stdQuadIndexCache[i] = pos;
			stdQuadIndexCache[i + 1] = pos + 1;
			stdQuadIndexCache[i + 2] = pos + 2;
			// CDA
			stdQuadIndexCache[i + 3] = pos + 2;
			stdQuadIndexCache[i + 4] = pos + 3;
			stdQuadIndexCache[i + 5] = pos;
			pos += 4;
		}
	}

	return stdQuadIndexCache.data();
}

void Painter::generateQuadIndices(size_t firstVertex, size_t numQuads)
{
	size_t numIndices = numQuads * 6;
	makeSpaceForPendingIndices(numIndices);

	unsigned short pos = static_cast<unsigned short>(firstVertex);
	for (size_t i = indicesPending; i < indicesPending + numIndices; i += 6) {
		// B-----C
		// |     |
		// A-----D
		// ABC
		indexBuffer[i] = pos;
		indexBuffer[i + 1] = pos + 1;
		indexBuffer[i + 2] = pos + 2;
		// CDA
		indexBuffer[i + 3] = pos + 2;
		indexBuffer[i + 4] = pos + 3;
		indexBuffer[i + 5] = pos;
		pos += 4;
	}

	indicesPending += numIndices;
}
