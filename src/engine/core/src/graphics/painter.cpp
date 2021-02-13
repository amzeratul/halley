#include "halley/core/graphics/painter.h"

#include <cassert>

#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <cstring> // memmove
#include <gsl/gsl_assert>

#include "halley/maths/polygon.h"
#include "resources/resources.h"

using namespace Halley;

struct LineVertex {
	Vector4f colour;
	Vector2f position;
	Vector2f normal;
	Vector2f width;
	char _padding[8];
};

Painter::Painter(Resources& resources)
	: halleyGlobalMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/MaterialBase"), true))
	, resources(resources)
	, solidLineMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/SolidLine")))
	, solidPolygonMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/SolidPolygon")))
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
	assert(camera->getZAngle().getRadians() == 0); // Camera rotation not accounted by following line
	auto camPos = camera->getPosition();
	return Rect4f(Vector2f(camPos.x, camPos.y) - size * 0.5f, size.x, size.y);
}

static Vector4f& getVertPos(char* vertexAttrib, size_t vertPosOffset)
{
	return *reinterpret_cast<Vector4f*>(vertexAttrib + vertPosOffset);
}

Painter::PainterVertexData Painter::addDrawData(const std::shared_ptr<Material>& material, size_t numVertices, size_t numIndices, bool standardQuadsOnly)
{
	updateClip();

	constexpr auto maxVertices = size_t(std::numeric_limits<IndexType>::max());
	if (numVertices > maxVertices) {
		throw Exception("Too many vertices in draw call: " + toString(numVertices) + ", maximum is " + toString(maxVertices), HalleyExceptions::Graphics);
	}
	if (verticesPending + numVertices > maxVertices) {
		flushPending();
	}

	Expects(material != nullptr);
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
	result.firstIndex = static_cast<IndexType>(verticesPending);

	indicesPending += numIndices;
	verticesPending += numVertices;
	bytesPending += result.dataSize;
	allIndicesAreQuads &= standardQuadsOnly;

	return result;
}

void Painter::draw(const std::shared_ptr<Material>& material, size_t numVertices, const void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType)
{
	Expects(primitiveType == PrimitiveType::Triangle);
	Expects(indices.size() % 3 == 0);

	const auto result = addDrawData(material, numVertices, indices.size(), false);

	memcpy(result.dstVertex, vertexData, result.dataSize);

	for (size_t i = 0; i < size_t(indices.size()); ++i) {
		result.dstIndex[i] = indices[i] + result.firstIndex;
	}
}

void Painter::drawQuads(const std::shared_ptr<Material>& material, size_t numVertices, const void* vertexData)
{
	Expects(numVertices % 4 == 0);
	Expects(vertexData != nullptr);

	const auto result = addDrawData(material, numVertices, numVertices * 3 / 2, true);

	memcpy(result.dstVertex, vertexData, result.dataSize);
	generateQuadIndices(result.firstIndex, numVertices / 4, result.dstIndex);
}

void Painter::drawSprites(const std::shared_ptr<Material>& material, size_t numSprites, const void* vertexData)
{
	Expects(vertexData != nullptr);

	const size_t verticesPerSprite = 4;
	const size_t numVertices = verticesPerSprite * numSprites;
	const size_t vertPosOffset = material->getDefinition().getVertexPosOffset();

	const auto result = addDrawData(material, numVertices, numSprites * 6, true);

	const char* const src = reinterpret_cast<const char*>(vertexData);

	for (size_t i = 0; i < numSprites; i++) {
		for (size_t j = 0; j < verticesPerSprite; j++) {
			const size_t srcOffset = i * result.vertexStride;
			const size_t dstOffset = (i * verticesPerSprite + j) * result.vertexStride;
			memcpy(result.dstVertex + dstOffset, src + srcOffset, result.vertexSize);

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

void Painter::drawSlicedSprite(const std::shared_ptr<Material>& material, Vector2f scale, Vector4f slices, const void* vertexData)
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

	const auto result = addDrawData(material, numVertices, numIndices, false);
	const char* const src = static_cast<const char*>(vertexData);

	// Vertices
	std::array<Vector2f, 4> pos = {{ Vector2f(0, 0), Vector2f(slices.x / scale.x, slices.y / scale.y), Vector2f(1 - slices.z / scale.x, 1 - slices.w / scale.y), Vector2f(1, 1) }};
	std::array<Vector2f, 4> tex = {{ Vector2f(0, 0), Vector2f(slices.x, slices.y), Vector2f(1 - slices.z, 1 - slices.w), Vector2f(1, 1) }};
	for (size_t i = 0; i < numVertices; i++) {
		const size_t ix = i & 3;
		const size_t iy = i >> 2;
		const size_t dstOffset = i * result.vertexStride;

		memcpy(result.dstVertex + dstOffset, src, result.vertexSize);

		Vector4f& vertPos = getVertPos(result.dstVertex + dstOffset, vertPosOffset);
		vertPos = Vector4f(pos[ix].x, pos[iy].y, tex[ix].x, tex[iy].y);
	}

	// Indices
	IndexType* dstIndex = result.dstIndex;
	for (size_t y = 0; y < 3; y++) {
		for (size_t x = 0; x < 3; x++) {
			generateQuadIndicesOffset(static_cast<IndexType>(result.firstIndex + x + (y * 4)), 4, dstIndex);
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

	const Vector4f col(colour.r, colour.g, colour.b, colour.a);

	constexpr float normalPos[] = { -1, 1, 1, -1 };
	constexpr size_t pointIdxOffset[] = { 0, 0, 1, 1 };

	const size_t nPoints = points.size();
	const size_t nSegments = (loop ? nPoints : (nPoints - 1));
	std::vector<LineVertex> vertices(nSegments * 4);

	auto segmentNormal = [&] (size_t i) -> std::optional<Vector2f>
	{
		if (!loop && i >= nSegments) {
			return {};
		} else {
			return (points[(i + 1) % nPoints] - points[i % nPoints]).normalized().orthoLeft();
		}
	};

	auto makeNormal = [] (Vector2f a, std::optional<Vector2f> maybeB) -> Vector2f
	{
		// Enabling this makes it looks nicer, but also introduces a lot of edge cases with acute angles that are very hard to deal with, so only enable for angles >= 90 degrees
		if (maybeB && maybeB.value().dot(a) >= -0.001f) {
			const auto b = maybeB.value();
			const auto c = (a + b).normalized();
			const auto cosHalfAlpha = c.dot(a);
			return c * (1.0f / cosHalfAlpha);
		} else {
			return a;
		}
	};

	std::optional<Vector2f> prevNormal = loop ? segmentNormal(nSegments - 1) : std::optional<Vector2f>();
	Vector2f normal = segmentNormal(0).value();

	for (size_t i = 0; i < nSegments; ++i) {
		std::optional<Vector2f> nextNormal = segmentNormal(i + 1);

		const Vector2f v0n = makeNormal(normal, prevNormal);
		const Vector2f v1n = makeNormal(normal, nextNormal);

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
			normal = nextNormal.value();
		}
	}

	drawQuads(material, vertices.size(), vertices.data());
}

static size_t getSegmentsForArc(float radius, float arcLen)
{
	return clamp(size_t(arcLen / float(pi() * 2) * 50.0f), size_t(4), size_t(256));
}

void Painter::drawCircle(Vector2f centre, float radius, float width, Colour4f colour, std::shared_ptr<Material> material)
{
	const size_t n = getSegmentsForArc(radius, 2 * float(pi()));
	std::vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(radius, 0).rotate(Angle1f::fromRadians(i * 2.0f * float(pi()) / n)));
	}
	drawLine(points, width, colour, true, std::move(material));
}

void Painter::drawCircleArc(Vector2f centre, float radius, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<Material> material)
{
	const float arcLen = (to - from).getRadians() + (from.turnSide(to) > 0 ? 0.0f : 0 * float(pi()));
	const size_t n = getSegmentsForArc(radius, arcLen);
	std::vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(radius, 0).rotate(from + Angle1f::fromRadians(i * arcLen / (n - 1))));
	}
	drawLine(points, width, colour, false, std::move(material));
}

void Painter::drawEllipse(Vector2f centre, Vector2f radius, float width, Colour4f colour, std::shared_ptr<Material> material)
{
	const size_t n = getSegmentsForArc(std::max(radius.x, radius.y), 2 * float(pi()));
	std::vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(1.0f, 0).rotate(Angle1f::fromRadians(i * 2.0f * float(pi()) / n)) * radius);
	}
	drawLine(points, width, colour, true, std::move(material));
}

void Painter::drawRect(Rect4f rect, float width, Colour4f colour, std::shared_ptr<Material> material)
{
	std::vector<Vector2f> points;
	points.push_back(rect.getTopLeft());
	points.push_back(rect.getTopRight());
	points.push_back(rect.getBottomRight());
	points.push_back(rect.getBottomLeft());
	drawLine(points, width, colour, true, std::move(material));
}

void Painter::drawPolygon(const Polygon& polygon, Colour4f colour, std::shared_ptr<Material> material)
{
	if (!polygon.isValid()) {
		return;
	}
	
	if (!material) {
		material = getSolidPolygonMaterial();
	}
	
	if (!polygon.isConvex()) {
		for (const auto& p: polygon.splitIntoConvex()) {
			drawPolygon(p, colour, material);
		}
		return;
	}

	auto col = Vector4f(colour.r, colour.g, colour.b, colour.a);
	
	const auto& vs = polygon.getVertices();
	const auto n = vs.size();
	std::vector<LineVertex> vertices(n);
	for (size_t i = 0; i < n; ++i) {
		vertices[i].position = vs[i];
		vertices[i].colour = col;
		vertices[i].normal = Vector2f();
		vertices[i].width = Vector2f();
	}
	std::vector<IndexType> indices((n - 2) * 3);
	for (size_t i = 0; i < n - 2; ++i) {
		indices.push_back(0);
		indices.push_back(static_cast<IndexType>(i) + 1);
		indices.push_back(static_cast<IndexType>(i) + 2);
	}

	draw(material, vertices.size(), vertices.data(), indices, PrimitiveType::Triangle);
}

void Painter::setLogging(bool logging)
{
	this->logging = logging;
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
	if (!activeRenderTarget) {
		throw Exception("No active render target", HalleyExceptions::Core);
	}
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
	pendingClip = rect;
}

void Painter::setClip()
{
	pendingClip = std::optional<Rect4i>();
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
	return solidLineMaterial;
}

std::shared_ptr<Material> Painter::getSolidPolygonMaterial()
{
	return solidPolygonMaterial;
}

void Painter::startDrawCall(const std::shared_ptr<Material>& material)
{
	constexpr bool enableDynamicBatching = true;

	if (material != materialPending) {
		if (!enableDynamicBatching || (materialPending != std::shared_ptr<Material>() && !(*material == *materialPending))) {
			flushPending();
		}
		materialPending = material;
	}
}

void Painter::flushPending()
{
	if (verticesPending > 0) {
		executeDrawPrimitives(*materialPending, verticesPending, vertexBuffer.data(), gsl::span<const IndexType>(indexBuffer.data(), indicesPending));
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

void Painter::executeDrawPrimitives(Material& material, size_t numVertices, void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType)
{
	Expects(primitiveType == PrimitiveType::Triangle);

	startDrawCall();

	// Load vertices
	// BAD: This method should take const IndexType*!
	setVertices(material.getDefinition(), numVertices, vertexData, indices.size(), const_cast<IndexType*>(indices.data()), allIndicesAreQuads);

	// Load material uniforms
	material.uploadData(*this);
	setMaterialData(material);

	// Go through each pass
	for (int i = 0; i < material.getDefinition().getNumPasses(); i++) {
		if (material.isPassEnabled(i)) {
			// Bind pass
			material.bind(i, *this);

			// Draw
			drawTriangles(indices.size());

			// Log stats
			if (logging) {
				nDrawCalls++;
				nTriangles += indices.size() / 3;
				nVertices += numVertices;
			}
		}
	}

	endDrawCall();
}

IndexType* Painter::getStandardQuadIndices(size_t numQuads)
{
	size_t sz = numQuads * 6;
	size_t oldSize = stdQuadIndexCache.size();

	if (oldSize < sz) {
		stdQuadIndexCache.resize(sz);
		IndexType pos = static_cast<IndexType>(oldSize * 2 / 3);
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

void Painter::generateQuadIndices(IndexType pos, size_t numQuads, IndexType* target)
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

void Painter::generateQuadIndicesOffset(IndexType pos, IndexType lineStride, IndexType* target)
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

	const auto oldHash = halleyGlobalMaterial->getHash();
	halleyGlobalMaterial->set("u_mvp", projection);
	halleyGlobalMaterial->set("u_viewPortSize", Vector2f(camera->getActiveViewPort().getSize()));
	if (oldHash != halleyGlobalMaterial->getHash()) {
		onUpdateProjection(*halleyGlobalMaterial);
	}
}

void Painter::updateClip()
{
	Rect4i finalRect = viewPort;
	if (pendingClip) {
		finalRect = (pendingClip.value() + viewPort.getTopLeft()).intersection(viewPort);
	}
	const Rect4i targetClip = getRectangleForActiveRenderTarget(finalRect);
	const bool enableClip = finalRect != activeRenderTarget->getViewPort();
	const std::optional<Rect4i> dstClip = enableClip ? targetClip : std::optional<Rect4i>();

	if (curClip != dstClip) {
		curClip = dstClip;

		flushPending();
		setClip(targetClip, enableClip);
	}
}
