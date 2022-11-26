#pragma once
#include "graphics_enums.h"
#include "render_context.h"
#include "material/material.h"

namespace Halley {
    class RenderSnapshot {
    public:
        void start();
        void end();

        void bind(RenderContext& context);
        void unbind(RenderContext& context);

        void setClip(Rect4i rect, bool enable);
    	void clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil);
	    void draw(const Material& material, size_t numVertices, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads);

        size_t getNumCommands() const;
        void playback(Painter& painter, std::optional<size_t> maxCommands);

    private:
        enum class CommandType : uint8_t {
	        Bind,
            Unbind,
            SetClip,
            Clear,
            Draw
        };

        struct BindData {
            Camera camera;
            RenderTarget* renderTarget;
        };

        struct SetClipData {
            Rect4i rect;
            bool enable;
        };

        struct ClearData {
            std::optional<Colour4f> colour;
            std::optional<float> depth;
            std::optional<uint8_t> stencil;
        };

        struct DrawData {
            std::shared_ptr<Material> material;
            size_t numVertices;
            Vector<char> vertexData;
            Vector<IndexType> indices;
            PrimitiveType primitive;
            bool allIndicesAreQuads;
        };

        Vector<Vector<std::pair<CommandType, uint16_t>>> commands;
        Vector<BindData> bindDatas;
        Vector<SetClipData> setClipDatas;
        Vector<ClearData> clearDatas;
        Vector<DrawData> drawDatas;

        Vector<std::pair<CommandType, uint16_t>>& getCurDrawCall();
        void finishDrawCall();

        void playBind(Painter& painter, BindData& data);
        void playUnbind(Painter& painter);
        void playClear(Painter& painter, ClearData& data);
        void playSetClip(Painter& painter, SetClipData& data);
        void playDraw(Painter& painter, DrawData& data);
    };
}
