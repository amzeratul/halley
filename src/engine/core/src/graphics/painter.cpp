#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <cstring> // memmove
#include <gsl/gsl_assert>
#include "resources/resources.h"

using namespace Halley;

Painter::Painter(Resources& resources)
	: resources(resources)
	, halleyGlobalMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/MaterialBase"), true))
{
}

Painter::~Painter()
{
}

void Painter::startRender()
{
	Material::resetBindCache();
	prevDrawCalls = nDrawCalls;
	prevTriangles = nTriangles;
	prevVertices = nVertices;
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

Painter::PainterVertexData Painter::addDrawData(std::shared_ptr<Material>& material, size_t numVertices, size_t numIndices, bool standardQuadsOnly)
{
	Expects(material);
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);

	startDrawCall(material);

	PainterVertexData result;

	result.vertexSize = material->getDefinition().getVertexSize();
	result.vertexStride = material->getDefinition().getVertexStride();
	result.dataSize = numVertices * result.vertexStride;
	makeSpaceForPendingVertices(result.dataSize);
	makeSpaceForPendingIndices(numIndices);

	result.dstVertex = vertexBuffer.data() + bytesPending;
	result.dstIndex = indexBuffer.data() + indicesPending;
	result.firstIndex = static_cast<unsigned short>(verticesPending);

	indicesPending += numIndices;
	verticesPending += numVertices;
	bytesPending += result.dataSize;
	allIndicesAreQuads &= standardQuadsOnly;

	return result;
}

void Painter::drawQuads(std::shared_ptr<Material> material, size_t numVertices, const void* vertexData)
{
	Expects(numVertices % 4 == 0);
	Expects(vertexData != nullptr);

	auto result = addDrawData(material, numVertices, numVertices * 3 / 2, true);

	memmove(result.dstVertex, vertexData, result.dataSize);
	generateQuadIndices(result.firstIndex, numVertices / 4, result.dstIndex);
}

void Painter::drawSprites(std::shared_ptr<Material> material, size_t numSprites, const void* vertexData)
{
	Expects(vertexData != nullptr);

	const size_t verticesPerSprite = 4;
	const size_t numVertices = verticesPerSprite * numSprites;
	const size_t vertPosOffset = material->getDefinition().getVertexPosOffset();

	auto result = addDrawData(material, numVertices, numSprites * 6, true);

	const char* const src = reinterpret_cast<const char*>(vertexData);

	for (size_t i = 0; i < numSprites; i++) {
		for (size_t j = 0; j < verticesPerSprite; j++) {
			size_t srcOffset = i * result.vertexStride;
			size_t dstOffset = (i * verticesPerSprite + j) * result.vertexStride;
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
	if (scale.x < 0.00001f || scale.y < 0.00001f) {
		//throw Exception("Scale is zero for material with texture " + material->getTexture(0)->getAssetId());
		return;
	}
	//Expects(scale.x > 0.0001f);
	//Expects(scale.y > 0.0001f);

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

	auto result = addDrawData(material, numVertices, numIndices, false);
	const char* const src = reinterpret_cast<const char*>(vertexData);

	// Vertices
	std::array<Vector2f, 4> pos = {{ Vector2f(0, 0), Vector2f(slices.x / scale.x, slices.y / scale.y), Vector2f(1 - slices.z / scale.x, 1 - slices.w / scale.y), Vector2f(1, 1) }};
	std::array<Vector2f, 4> tex = {{ Vector2f(0, 0), Vector2f(slices.x, slices.y), Vector2f(1 - slices.z, 1 - slices.w), Vector2f(1, 1) }};
	for (size_t i = 0; i < numVertices; i++) {
		const size_t ix = i & 3;
		const size_t iy = i >> 2;
		const size_t dstOffset = i * result.vertexStride;

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

void Painter::drawLine(gsl::span<const Vector2f> points, float width, Colour4f colour, bool loop, std::shared_ptr<Material> material)
{
	if (!material) {
		material = getSolidLineMaterial();
	}

	// Need at least two points to draw a line
	if (points.size() < 2) {
		return;
	}

	struct LineVertex {
		Vector4f colour;
		Vector2f position;
		Vector2f normal;
		Vector2f width;
		char _padding[8];
	};

	const Vector4f col(colour.r, colour.g, colour.b, colour.a);

	constexpr float normalPos[] = { -1, 1, 1, -1 };
	constexpr size_t pointIdxOffset[] = { 0, 0, 1, 1 };

	const size_t nPoints = points.size();
	const size_t nSegments = (loop ? nPoints : (nPoints - 1));
	std::vector<LineVertex> vertices(nSegments * 4);

	auto segmentNormal = [&] (size_t i) -> Maybe<Vector2f>
	{
		if (!loop && i >= nSegments) {
			return {};
		} else {
			return (points[(i + 1) % nPoints] - points[i % nPoints]).normalized().orthoLeft();
		}
	};

	auto makeNormal = [] (Vector2f a, Maybe<Vector2f> maybeB) -> Vector2f
	{
		if (maybeB) {
			const auto b = maybeB.get();
			const auto c = (a + b).normalized();
			const auto cosHalfAlpha = c.dot(a);
			return c * (1.0f / cosHalfAlpha);
		} else {
			return a;
		}
	};

	Maybe<Vector2f> prevNormal = loop ? segmentNormal(nSegments - 1) : Maybe<Vector2f>();
	Vector2f normal = segmentNormal(0).get();
	Maybe<Vector2f> nextNormal;

	for (size_t i = 0; i < nSegments; ++i) {
		nextNormal = segmentNormal(i + 1);

		Vector2f v0n = makeNormal(normal, prevNormal);
		Vector2f v1n = makeNormal(normal, nextNormal);

		for (size_t j = 0; j < 4; ++j) {
			const size_t idx = i * 4 + j;
			auto& v = vertices[idx];
			v.colour = col;
			v.position = points[(i + pointIdxOffset[j]) % nPoints];
			v.normal = j <= 1 ? v0n : v1n;
			v.width.x = width;
			v.width.y = normalPos[j];
		}

		prevNormal = normal;
		if (nextNormal) {
			normal = nextNormal.get();
		}
	}

	drawQuads(material, vertices.size(), vertices.data());
}

void Painter::drawCircle(Vector2f centre, float radius, float width, Colour4f colour, std::shared_ptr<Material> material)
{
	const size_t n = clamp(size_t(radius / 2), size_t(16), size_t(256));
	std::vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(radius, 0).rotate(Angle1f::fromRadians(i * 2.0f * float(pi()) / n)));
	}
	drawLine(points, width, colour, true, material);
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
	activeRenderTarget = &camera->getActiveRenderTarget();
	activeRenderTarget->onBind(*this);

	// Set viewport
	viewPort = camera->getActiveViewPort();
	setViewPort(getRectangleForActiveRenderTarget(viewPort));
	setClip();

	// Update projection
	updateProjection();
}

void Painter::unbind(RenderContext& context)
{
	flush();
	activeRenderTarget->onUnbind(*this);
	activeRenderTarget = nullptr;
	camera->rendering = false;
}

void Painter::setRelativeClip(Rect4f rect)
{
	std::array<Vector2f, 4> ps = {{ rect.getTopLeft(), rect.getTopRight(), rect.getBottomLeft(), rect.getBottomRight() }};
	float x0 = -std::numeric_limits<float>::infinity();
	float x1 = std::numeric_limits<float>::infinity();
	float y0 = -std::numeric_limits<float>::infinity();
	float y1 = std::numeric_limits<float>::infinity();
	for (auto& p: ps) {
		auto point = camera->worldToScreen(p, Rect4f(viewPort));
		x0 = std::max(x0, point.x);
		x1 = std::min(x1, point.x);
		y0 = std::max(y0, point.y);
		y1 = std::min(y1, point.y);
	}
	setClip(Rect4i(Vector2i(Vector2f(x0, y0).floor()), Vector2i(Vector2f(x1, y1).ceil())));
}

void Painter::setClip(Rect4i rect)
{
	flushPending();
	Rect4i finalRect = (rect + viewPort.getTopLeft()).intersection(viewPort);
	setClip(getRectangleForActiveRenderTarget(finalRect), finalRect != activeRenderTarget->getViewPort());
}

void Painter::setClip()
{
	flushPending();
	setClip(getRectangleForActiveRenderTarget(viewPort), viewPort != activeRenderTarget->getViewPort());
}

Rect4i Painter::getRectangleForActiveRenderTarget(Rect4i r)
{
	Expects(activeRenderTarget);
	int h = activeRenderTarget->getViewPort().getHeight();
	if (activeRenderTarget->getViewportFlipVertical()) {
		int y = h - r.getBottom();
		return Rect4i(Vector2i(r.getLeft(), y), r.getWidth(), r.getHeight());
	} else {
		return r;
	}
}

std::shared_ptr<Material> Painter::getSolidLineMaterial()
{
	if (!solidLineMaterial) {
		solidLineMaterial = std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/SolidLine"));
	}
	return solidLineMaterial;
}

void Painter::startDrawCall(std::shared_ptr<Material>& material)
{
	if (material != materialPending) {
		if (materialPending != std::shared_ptr<Material>() && !(*material == *materialPending)) {
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
	allIndicesAreQuads = true;
	if (materialPending) {
		Material::resetBindCache();
		materialPending.reset();
	}
}

void Painter::executeDrawTriangles(Material& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices)
{
	startDrawCall();

	// Load vertices
	setVertices(material.getDefinition(), numVertices, vertexData, numIndices, indices, allIndicesAreQuads);

	// Load material uniforms
	material.uploadData(*this);
	setMaterialData(material);

	// Go through each pass
	for (int i = 0; i < material.getDefinition().getNumPasses(); i++) {
		if (material.isPassEnabled(i)) {
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

	endDrawCall();
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

RenderTarget& Painter::getActiveRenderTarget()
{
	Expects(activeRenderTarget);
	return *activeRenderTarget;
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

void Painter::updateProjection()
{
	camera->updateProjection(activeRenderTarget->getProjectionFlipVertical());
	projection = camera->getProjection();
	
	auto old = halleyGlobalMaterial->clone();
	halleyGlobalMaterial->set("u_mvp", projection);
	if (*old != *halleyGlobalMaterial) {
		onUpdateProjection(*halleyGlobalMaterial);
	}
}
