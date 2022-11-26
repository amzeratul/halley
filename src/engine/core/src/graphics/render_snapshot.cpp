#include "halley/core/graphics/render_snapshot.h"
using namespace Halley;

void RenderSnapshot::start()
{
	commands.clear();
	bindDatas.clear();
	setClipDatas.clear();
	clearDatas.clear();
	drawDatas.clear();
}

void RenderSnapshot::end()
{
	if (!commands.empty() && commands.back().first == CommandType::Bind) {
		commands.pop_back();
	}
}

void RenderSnapshot::bind(RenderContext& context)
{
	commands.emplace_back(CommandType::Bind, static_cast<uint16_t>(bindDatas.size()));
	bindDatas.push_back(BindData{ context.getCamera(), &context.getDefaultRenderTarget() });
}

void RenderSnapshot::unbind(RenderContext& context)
{
	commands.emplace_back(CommandType::Unbind, 0);
}

void RenderSnapshot::setClip(Rect4i rect, bool enable)
{
	commands.emplace_back(CommandType::SetClip, static_cast<uint16_t>(setClipDatas.size()));
	setClipDatas.push_back(SetClipData{ rect, enable });
}

void RenderSnapshot::clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	commands.emplace_back(CommandType::Clear, static_cast<uint16_t>(clearDatas.size()));
	clearDatas.push_back(ClearData{ colour, depth, stencil });
}

void RenderSnapshot::draw(const Material& material, size_t numVertices, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads)
{
	commands.emplace_back(CommandType::Draw, static_cast<uint16_t>(drawDatas.size()));
	drawDatas.push_back(DrawData{ material.clone(), numVertices, Vector<char>(vertexData.begin(), vertexData.end()), Vector<IndexType>(indices.begin(), indices.end()), primitive, allIndicesAreQuads });
}

size_t RenderSnapshot::getNumCommands() const
{
	return commands.size();
}

void RenderSnapshot::playback(Painter& painter, std::optional<size_t> maxCommands)
{
	painter.stopRecording();

	const size_t n = std::min(commands.size(), maxCommands.value_or(commands.size()));
	for (size_t i = 0; i < n; ++i) {
		const auto type = commands[i].first;
		const auto idx = commands[i].second;

		switch (type) {
		case CommandType::Bind:
			playBind(painter, bindDatas[idx]);
			break;

		case CommandType::Unbind:
			playUnbind(painter);
			break;

		case CommandType::Clear:
			playClear(painter, clearDatas[idx]);
			break;

		case CommandType::SetClip:
			playSetClip(painter, setClipDatas[idx]);
			break;

		case CommandType::Draw:
			playDraw(painter, drawDatas[idx]);
			break;
		}
	}
}

void RenderSnapshot::playBind(Painter& painter, BindData& data)
{
	painter.doBind(data.camera, *data.renderTarget);
}

void RenderSnapshot::playUnbind(Painter& painter)
{
	painter.doUnbind();
}

void RenderSnapshot::playClear(Painter& painter, ClearData& data)
{
	painter.clear(data.colour, data.depth, data.stencil);
}

void RenderSnapshot::playSetClip(Painter& painter, SetClipData& data)
{
	painter.setClip(data.rect, data.enable);
}

void RenderSnapshot::playDraw(Painter& painter, DrawData& data)
{
	painter.executeDrawPrimitives(*data.material, data.numVertices, data.vertexData, data.indices, data.primitive, data.allIndicesAreQuads);
}
