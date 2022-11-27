#include "halley/core/graphics/render_snapshot.h"

#include "graphics/material/material_definition.h"
#include "graphics/render_target/render_target_texture.h"
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
	if (!commands.empty()) {
		if (commands.back().empty()) {
			commands.pop_back();
		} else {
			const auto lastType = commands.back().back().first;
			if (lastType != CommandType::Draw && lastType != CommandType::Clear) {
				commands.pop_back();
			}
		}
	}
}

void RenderSnapshot::bind(RenderContext& context)
{
	getCurDrawCall().emplace_back(CommandType::Bind, static_cast<uint16_t>(bindDatas.size()));
	bindDatas.push_back(BindData{ context.getCamera(), &context.getDefaultRenderTarget() });
}

void RenderSnapshot::unbind(RenderContext& context)
{
	getCurDrawCall().emplace_back(CommandType::Unbind, 0);
}

void RenderSnapshot::setClip(Rect4i rect, bool enable)
{
	getCurDrawCall().emplace_back(CommandType::SetClip, static_cast<uint16_t>(setClipDatas.size()));
	setClipDatas.push_back(SetClipData{ rect, enable });
}

void RenderSnapshot::clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	getCurDrawCall().emplace_back(CommandType::Clear, static_cast<uint16_t>(clearDatas.size()));
	clearDatas.push_back(ClearData{ colour, depth, stencil });
	finishDrawCall();
}

void RenderSnapshot::draw(const Material& material, size_t numVertices, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads)
{
	getCurDrawCall().emplace_back(CommandType::Draw, static_cast<uint16_t>(drawDatas.size()));
	drawDatas.push_back(DrawData{ material.clone(), numVertices, Vector<char>(vertexData.begin(), vertexData.end()), Vector<IndexType>(indices.begin(), indices.end()), primitive, allIndicesAreQuads });
	finishDrawCall();
}

size_t RenderSnapshot::getNumCommands() const
{
	return commands.size();
}

RenderSnapshot::CommandInfo RenderSnapshot::getCommandInfo(size_t commandIdx) const
{
	CommandInfo result;

	const auto& command = commands.at(commandIdx);
	result.type = command.empty() ? CommandType::Undefined : command.back().first;

	for (auto& entry: command) {
		if (entry.first == CommandType::Bind || entry.first == CommandType::Unbind) {
			result.hasBindChange = true;
		}
		if (entry.first == CommandType::SetClip) {
			result.hasClipChange = true;
		}
	}

	if (commandIdx == 0) {
		result.reason = Reason::First;
	} else {
		if (result.type == CommandType::Clear) {
			result.reason = Reason::Clear;
		} else if (result.type == CommandType::Draw) {
			if (result.hasBindChange) {
				result.reason = Reason::ChangeBind;
			} else if (result.hasClipChange) {
				result.reason = Reason::ChangeClip;
			} else {
				const auto& prev = commands.at(commandIdx - 1);
				const auto prevType = prev.empty() ? CommandType::Undefined : prev.back().first;
				if (prevType == CommandType::Clear) {
					result.reason = Reason::AfterClear;
				} else if (prevType == CommandType::Draw) {
					const auto& curDraw = drawDatas[commands.at(commandIdx).back().second];
					const auto& prevDraw = drawDatas[commands.at(commandIdx - 1).back().second];

					if (curDraw.material->getDefinition().getName() != prevDraw.material->getDefinition().getName()) {
						result.reason = Reason::MaterialDefinition;
					} else if (curDraw.material->getTextures() != prevDraw.material->getTextures()) {
						result.reason = Reason::Textures;
					} else if (curDraw.material != prevDraw.material) {
						result.reason = Reason::MaterialParameters;
					}
				}
			}
		}
	}

	if (result.type == CommandType::Clear) {
		const auto& curClear = clearDatas[commands.at(commandIdx).back().second];
		result.clearData = curClear;
	} else if (result.type == CommandType::Draw) {
		const auto& curDraw = drawDatas[commands.at(commandIdx).back().second];
		result.materialDefinition = curDraw.material->getDefinition().getName();
		result.materialHash = curDraw.material->getHash();
		for (const auto& tex: curDraw.material->getTextures()) {
			result.textures.push_back(tex->getAssetId());
		}
	}

	return result;
}

void RenderSnapshot::playback(Painter& painter, std::optional<size_t> maxCommands) const
{
	painter.stopRecording();

	const auto startCamera = painter.camera;
	const auto startRenderTarget = painter.activeRenderTarget;

	painter.setClip();
	painter.updateClip();

	const size_t n = std::min(commands.size(), maxCommands.value_or(commands.size()));

	for (size_t i = 0; i < n; ++i) {
		for (const auto& command: commands[i]) {
			const auto type = command.first;
			const auto idx = command.second;

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

	const auto finalRenderTarget = painter.activeRenderTarget;
	painter.doUnbind();
	painter.doBind(startCamera, *startRenderTarget);
	painter.setClip(Rect4i(), false);

	if (startRenderTarget != finalRenderTarget) {
		const auto* texRenderTarget = dynamic_cast<TextureRenderTarget*>(finalRenderTarget);
		if (texRenderTarget) {
			painter.blitTexture(texRenderTarget->getTexture(0));
		}
	}
}

Vector<std::pair<RenderSnapshot::CommandType, uint16_t>>& RenderSnapshot::getCurDrawCall()
{
	if (commands.empty()) {
		commands.emplace_back();
	}
	return commands.back();
}

void RenderSnapshot::finishDrawCall()
{
	commands.emplace_back();
}

void RenderSnapshot::playBind(Painter& painter, const BindData& data) const
{
	painter.doBind(data.camera, *data.renderTarget);
}

void RenderSnapshot::playUnbind(Painter& painter) const
{
	painter.doUnbind();
}

void RenderSnapshot::playClear(Painter& painter, const ClearData& data) const
{
	painter.clear(data.colour, data.depth, data.stencil);
}

void RenderSnapshot::playSetClip(Painter& painter, const SetClipData& data) const
{
	painter.setClip(data.rect, data.enable);
}

void RenderSnapshot::playDraw(Painter& painter, const DrawData& data) const
{
	painter.executeDrawPrimitives(*data.material, data.numVertices, data.vertexData, data.indices, data.primitive, data.allIndicesAreQuads);
}
