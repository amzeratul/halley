#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <cstring> // memmove
#include <gsl/gsl_assert>

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

Rect4f Painter::getWorldViewAABB() const
{
	Vector2f size = Vector2f(viewPort.getSize()) / camera->getZoom();
	assert(camera->getAngle().getRadians() == 0); // Camera rotation not accounted by following line
	return Rect4f(camera->getPosition() - size * 0.5f, size);
}

static Vector4f& getVertPos(char* vertexAttrib, size_t vertPosOffset)
{
	return *reinterpret_cast<Vector4f*>(vertexAttrib + vertPosOffset);
}

Painter::PainterVertexData Painter::addDrawData(std::shared_ptr<Material>& material, size_t numVertices, size_t numIndices)
{
	Expects(material);
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);

	startDrawCall(material);

	PainterVertexData result;

	result.vertexSize = material->getDefinition().getVertexStride();
	result.dataSize = numVertices * result.vertexSize;
	makeSpaceForPendingVertices(result.dataSize);
	makeSpaceForPendingIndices(numIndices);

	result.dstVertex = vertexBuffer.data() + bytesPending;
	result.dstIndex = indexBuffer.data() + indicesPending;
	result.firstIndex = static_cast<unsigned short>(verticesPending);

	indicesPending += numIndices;
	verticesPending += numVertices;
	bytesPending += result.dataSize;

	return result;
}

void Painter::drawQuads(std::shared_ptr<Material> material, size_t numVertices, const void* vertexData)
{
	Expects(numVertices % 4 == 0);
	Expects(vertexData != nullptr);

	auto result = addDrawData(material, numVertices, numVertices * 3 / 2);

	memmove(result.dstVertex, vertexData, result.dataSize);
	generateQuadIndices(result.firstIndex, numVertices / 4, result.dstIndex);
}

void Painter::drawSprites(std::shared_ptr<Material> material, size_t numSprites, const void* vertexData)
{
	Expects(vertexData != nullptr);

	const size_t verticesPerSprite = 4;
	const size_t numVertices = verticesPerSprite * numSprites;
	const size_t vertPosOffset = material->getDefinition().getVertexPosOffset();

	auto result = addDrawData(material, numVertices, numSprites * 6);

	const char* const src = reinterpret_cast<const char*>(vertexData);

	for (size_t i = 0; i < numSprites; i++) {
		for (size_t j = 0; j < verticesPerSprite; j++) {
			size_t srcOffset = i * result.vertexSize;
			size_t dstOffset = (i * verticesPerSprite + j) * result.vertexSize;
			memmove(result.dstVertex + dstOffset, src + srcOffset, result.vertexSize);

			// j -> vertPos
			// 0 -> 0, 0
			// 1 -> 1, 0
			// 2 -> 1, 1
			// 3 -> 0, 1
			const float x = ((j & 1) ^ ((j & 2) >> 1)) * 1.0f;
			const float y = ((j & 2) >> 1) * 1.0f;
			getVertPos(result.dstVertex + dstOffset, vertPosOffset) = Vector4f(x, y, x, y);
		}
	}

	generateQuadIndices(result.firstIndex, numSprites, result.dstIndex);
}

void Painter::drawSlicedSprite(std::shared_ptr<Material> material, Vector2f scale, Vector4f slices, const void* vertexData)
{
	Expects(vertexData != nullptr);
	Expects(scale.x > 0.0001f);
	Expects(scale.y > 0.0001f);

	//         a        c
	//   00 -- 01 ----- 02 -- 03
	//   |     |        |     |
	// b 04 -- 05 ----- 06 -- 07
	//   |     |        |     |
	//   |     |        |     |
	// d 08 -- 09 ----- 10 -- 11
	//   |     |        |     |
	//   12 -- 13 ----- 14 -- 15

	const size_t numVertices = 16;
	const size_t numIndices = 9 * 6; // 9 quads, 6 indices per quad
	const size_t vertPosOffset = material->getDefinition().getVertexPosOffset();

	auto result = addDrawData(material, numVertices, numIndices);
	const char* const src = reinterpret_cast<const char*>(vertexData);

	// Vertices
	std::array<Vector2f, 4> pos = { Vector2f(0, 0), Vector2f(slices.x / scale.x, slices.y / scale.y), Vector2f(1 - slices.z / scale.x, 1 - slices.w / scale.y), Vector2f(1, 1) };
	std::array<Vector2f, 4> tex = { Vector2f(0, 0), Vector2f(slices.x, slices.y), Vector2f(1 - slices.z, 1 - slices.w), Vector2f(1, 1) };
	for (size_t i = 0; i < numVertices; i++) {
		const size_t ix = i & 3;
		const size_t iy = i >> 2;
		const size_t dstOffset = i * result.vertexSize;

		memmove(result.dstVertex + dstOffset, src, result.vertexSize);

		Vector4f& vertPos = getVertPos(result.dstVertex + dstOffset, vertPosOffset);
		vertPos = Vector4f(pos[ix].x, pos[iy].y, tex[ix].x, tex[iy].y);
	}

	// Indices
	unsigned short* dstIndex = result.dstIndex;
	for (size_t y = 0; y < 3; y++) {
		for (size_t x = 0; x < 3; x++) {
			generateQuadIndicesOffset(static_cast<unsigned short>(result.firstIndex + x + (y * 4)), 4, dstIndex);
			dstIndex += 6;
		}
	}
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
	// Setup camera
	camera = &context.getCamera();
	camera->rendering = true;
	camera->defaultRenderTarget = &context.getDefaultRenderTarget();

	// Set render target
	auto& rt = camera->getActiveRenderTarget();
	renderTargetViewPort = rt.getViewPort();
	renderTargetIsScreen = rt.isScreen();
	rt.bind();

	// Set viewport
	viewPort = camera->getActiveViewPort();
	setViewPort(viewPort, renderTargetViewPort.getSize(), renderTargetIsScreen);
	setClip();

	// Update projection
	camera->updateProjection(renderTargetIsScreen);
	projection = camera->getProjection();
}

void Painter::unbind(RenderContext& context)
{
	flush();
	camera->getActiveRenderTarget().unbind();
	camera->rendering = false;
}

void Painter::setClip(Rect4i rect)
{
	flushPending();
	Rect4i finalRect = (rect + viewPort.getTopLeft()).intersection(viewPort);
	setClip(finalRect, renderTargetViewPort.getSize(), finalRect != renderTargetViewPort, renderTargetIsScreen);
}

void Painter::setClip()
{
	flushPending();
	setClip(viewPort, renderTargetViewPort.getSize(), viewPort != renderTargetViewPort, renderTargetIsScreen);
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
	if (materialPending) {
		Material::resetBindCache();
		materialPending.reset();
	}
}

void Painter::executeDrawTriangles(Material& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices)
{
	// Bind projection
	material.set("u_mvp", projection);

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
			// A-----B
			// |     |
			// D-----C
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

void Painter::generateQuadIndices(unsigned short pos, size_t numQuads, unsigned short* target)
{
	size_t numIndices = numQuads * 6;
	for (size_t i = 0; i < numIndices; i += 6) {
		// A-----B
		// |     |
		// D-----C
		// ABC
		target[i] = pos;
		target[i + 1] = pos + 1;
		target[i + 2] = pos + 2;
		// CDA
		target[i + 3] = pos + 2;
		target[i + 4] = pos + 3;
		target[i + 5] = pos;
		pos += 4;
	}
}

void Painter::generateQuadIndicesOffset(unsigned short pos, unsigned short lineStride, unsigned short* target)
{
	// A-----B
	// |     |
	// C-----D
	// ABD
	target[0] = pos;
	target[1] = pos + 1;
	target[2] = pos + lineStride + 1;
	// DCA
	target[3] = pos + lineStride + 1;
	target[4] = pos + lineStride;
	target[5] = pos;
}
