#pragma once
#include "camera.h"
#include "blend.h"
#include "halley/maths/colour.h"
#include "graphics_enums.h"
#include <condition_variable>
#include <halley/maths/vector4.h>

#include "halley/maths/circle.h"

namespace Halley
{
	class BezierCubic;
	class BezierQuadratic;
	class Polygon;
	class Resources;
	class Shader;
	class Material;
	class MaterialDefinition;
	class Camera;
	class RenderContext;
	class Core;

	class Painter
	{
		friend class RenderContext;
		friend class Core;

		struct PainterVertexData
		{
			char* dstVertex;
			IndexType* dstIndex;
			size_t vertexSize;
			size_t vertexStride;
			size_t dataSize;
			IndexType firstIndex;
		};

	public:
		Painter(Resources& resources);
		virtual ~Painter();

		void flush();

		Rect4i getViewPort() const { return viewPort; }
		const Camera& getCurrentCamera() const { return camera; }
		Rect4f getWorldViewAABB() const;

		virtual void clear(std::optional<Colour> colour, std::optional<float> depth = 1.0f, std::optional<uint8_t> stencil = 0) = 0;
		virtual void setMaterialPass(const Material& material, int pass) = 0;
		virtual void setMaterialData(const Material& material) = 0;

		void setRelativeClip(Rect4f rect);
		void setClip(Rect4i rect);
		void setClip();

		// Draws primitives
		void draw(const std::shared_ptr<Material>& material, size_t numVertices, const void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType = PrimitiveType::Triangle);

		// Draws quads
		void drawQuads(const std::shared_ptr<Material>& material, size_t numVertices, const void* vertexData);

		// Draw sprites takes a single vertex per sprite, duplicates the data across multiple vertices, and draws
		// vertPosOffset is the offset, in bytes, from the start of each vertex's data, to a Vector2f which will be filled with the vertex's position in 0-1 space.
		void drawSprites(const std::shared_ptr<Material>& material, size_t numSprites, const void* vertexData);

		// Draw one sliced sprite. Slices -> x = left, y = top, z = right, w = bottom, in [0..1] space relative to the texture
		void drawSlicedSprite(const std::shared_ptr<Material>& material, Vector2f scale, Vector4f slices, const void* vertexData);

		// Draws a line across all points (if no material is specified, use standard one)
		void drawLine(gsl::span<const Vector2f> points, float width, Colour4f colour, bool loop = false, std::shared_ptr<Material> material = {});
		void drawLine(const BezierQuadratic& bezier, float width, Colour4f colour, std::shared_ptr<Material> material = {});
		void drawLine(const BezierCubic& bezier, float width, Colour4f colour, std::shared_ptr<Material> material = {});

		// Circle drawing
		void drawCircle(Vector2f centre, float radius, float width, Colour4f colour, std::shared_ptr<Material> material = {});
		void drawCircle(Circle circle, float width, Colour4f colour, std::shared_ptr<Material> material = {});
		void drawCircleArc(Vector2f centre, float radius, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<Material> material = {});
		void drawCircleArc(Circle circle, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<Material> material = {});
		void drawEllipse(Vector2f centre, Vector2f radius, float width, Colour4f colour, std::shared_ptr<Material> material = {});

		// Rect drawing
		void drawRect(Rect4f rect, float width, Colour4f colour, std::shared_ptr<Material> material = {});

		// Polygon drawing
		void drawPolygon(const Polygon& polygon, Colour4f colour, std::shared_ptr<Material> material = {});

		size_t getNumDrawCalls() const { return nDrawCalls; }
		size_t getNumVertices() const { return nVertices; }
		size_t getNumTriangles() const { return nTriangles; }

		size_t getPrevDrawCalls() const { return prevDrawCalls; }
		size_t getPrevVertices() const { return prevVertices; }
		size_t getPrevTriangles() const { return prevTriangles; }

		void setLogging(bool logging);

	protected:
		virtual void startDrawCall() {}
		virtual void endDrawCall() {}
		virtual void doStartRender() = 0;
		virtual void doEndRender() = 0;
		virtual void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, IndexType* indices, bool standardQuadsOnly) = 0;
		virtual void drawTriangles(size_t numIndices) = 0;

		virtual void setViewPort(Rect4i rect) = 0;
		virtual void setClip(Rect4i clip, bool enable) = 0;

		virtual void onUpdateProjection(Material& material) = 0;
		void generateQuadIndices(IndexType firstVertex, size_t numQuads, IndexType* target);
		RenderTarget& getActiveRenderTarget();

		std::unique_ptr<Material> halleyGlobalMaterial;

	private:
		Resources& resources;
		RenderContext* activeContext = nullptr;
		RenderTarget* activeRenderTarget = nullptr;
		Matrix4f projection;
		Rect4i viewPort;
		Camera camera;

		size_t verticesPending = 0;
		size_t bytesPending = 0;
		size_t indicesPending = 0;
		bool allIndicesAreQuads = true;
		Vector<char> vertexBuffer;
		Vector<IndexType> indexBuffer;
		std::shared_ptr<Material> materialPending;
		std::shared_ptr<Material> solidLineMaterial;
		std::shared_ptr<Material> solidPolygonMaterial;

		size_t nDrawCalls = 0;
		size_t nVertices = 0;
		size_t nTriangles = 0;
		size_t prevDrawCalls = 0;
		size_t prevVertices = 0;
		size_t prevTriangles = 0;
		bool logging = true;

		Vector<IndexType> stdQuadIndexCache;
		std::optional<Rect4i> curClip;
		std::optional<Rect4i> pendingClip;

		void bind(RenderContext& context);
		void unbind(RenderContext& context);
		
		void startRender();
		void endRender();
		
		void resetPending();
		void startDrawCall(const std::shared_ptr<Material>& material);
		void flushPending();
		void executeDrawPrimitives(Material& material, size_t numVertices, void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType = PrimitiveType::Triangle);

		void makeSpaceForPendingVertices(size_t numBytes);
		void makeSpaceForPendingIndices(size_t numIndices);
		PainterVertexData addDrawData(const std::shared_ptr<Material>& material, size_t numVertices, size_t numIndices, bool standardQuadsOnly);

		IndexType* getStandardQuadIndices(size_t numQuads);
		void generateQuadIndicesOffset(IndexType firstVertex, IndexType lineStride, IndexType* target);

		void updateProjection();
		void updateClip();

		Rect4i getRectangleForActiveRenderTarget(Rect4i rectangle);

		std::shared_ptr<Material> getSolidLineMaterial();
		std::shared_ptr<Material> getSolidPolygonMaterial();
	};
}
