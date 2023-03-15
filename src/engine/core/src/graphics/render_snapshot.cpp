#include "halley/graphics/render_snapshot.h"

#include "halley/graphics/render_context.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/support/logger.h"
using namespace Halley;

RenderSnapshot::RenderSnapshot()
	: pendingTimestamps(0)
{
}

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
	drawDatas.push_back(DrawData{ &material, {}, numVertices, Vector<char>(vertexData.begin(), vertexData.end()), Vector<IndexType>(indices.begin(), indices.end()), primitive, allIndicesAreQuads });
	finishDrawCall();
}

void RenderSnapshot::finish()
{
	for (auto& drawData: drawDatas) {
		drawData.material = drawData.materialTemp->clone();
	}
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

	auto getLastDraw = [&]() -> const DrawData*
	{
		for (int i = static_cast<int>(commandIdx); --i >= 0; ) {
			if (commands.at(i).back().first == CommandType::Draw) {
				return &drawDatas[commands.at(i).back().second];
			}
		}
		return nullptr;
	};

	if (result.type == CommandType::Clear) {
		const auto& curClear = clearDatas[commands.at(commandIdx).back().second];
		result.clearData = curClear;
	} else if (result.type == CommandType::Draw) {
		const auto& curDraw = drawDatas[commands.at(commandIdx).back().second];
		const auto curMat = curDraw.material;
		result.materialDefinition = curMat->getDefinition().getName();
		result.materialHash = curMat->getPartialHash();
		for (size_t i = 0; i < curMat->getNumTextureUnits(); ++i) {
			result.textures.push_back(curMat->getRawTexture(static_cast<int>(i))->getAssetId());
		}
		result.numTriangles = curDraw.indices.size() / 3;

		if (const auto* prevDraw = getLastDraw()) {
			const auto prevMat = prevDraw->material;
			result.hasMaterialDefChange = curMat->getDefinition().getName() != prevMat->getDefinition().getName();
			result.hasTextureChange = curMat->getTextures() != prevMat->getTextures();
			result.hasMaterialParamsChange = curMat->getDataBlocks() != prevMat->getDataBlocks()
				|| curMat->getPassesEnabled() != prevMat->getPassesEnabled()
				|| curMat->getStencilReferenceOverride() != prevMat->getStencilReferenceOverride();
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
					if (result.hasMaterialDefChange) {
						result.reason = Reason::MaterialDefinition;
					} else if (result.hasMaterialParamsChange) {
						result.reason = Reason::MaterialParameters;
					} else if (result.hasTextureChange) {
						result.reason = Reason::Textures;
					}
				}
			}
		}
	}

	return result;
}

RenderSnapshot::PlaybackResult RenderSnapshot::playback(Painter& painter, std::optional<size_t> maxCommands, TargetBufferType blitType, std::shared_ptr<const MaterialDefinition> debugMaterial) const
{
	painter.stopRecording();

	const auto startCamera = painter.camera;
	const auto startRenderTarget = painter.activeRenderTarget;

	painter.resetState();

	const size_t n = std::min(commands.size(), maxCommands.value_or(commands.size()));

	for (size_t i = 0; i < n; ++i) {
		for (const auto& command: commands[i]) {
			const auto type = command.first;
			const auto idx = command.second;
			const bool last = i == n - 1;

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
				playDraw(painter, drawDatas[idx], {});
				if (last && debugMaterial) {
					playDraw(painter, drawDatas[idx], debugMaterial);
				}
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
			if (blitType == TargetBufferType::Depth) {
				painter.blitTexture(texRenderTarget->getDepthTexture(), blitType);
			} else {
				painter.blitTexture(texRenderTarget->getTexture(0), blitType);
			}
		}
	}

	return PlaybackResult{ finalRenderTarget ? finalRenderTarget->getName() : "" };
}

void RenderSnapshot::addPendingTimestamp()
{
	++pendingTimestamps;
}

void RenderSnapshot::onTimestamp(TimestampType type, size_t idx, uint64_t value)
{
	if (timestamps.size() <= idx) {
		timestamps.reserve(nextPowerOf2(idx + 1));
		timestamps.resize(idx + 1);
	}

	if (type == TimestampType::FrameStart) {
		startTime = value;
	} else if (type == TimestampType::FrameEnd) {
		endTime = value;
	} else if (type == TimestampType::CommandStart) {
		if (timestamps[idx].setupDone == 0) {
			timestamps[idx].setupDone = value;
		}
		timestamps[idx].start = value;
	} else if (type == TimestampType::CommandEnd) {
		timestamps[idx].end = value;
	} else if (type == TimestampType::CommandSetupDone) {
		timestamps[idx].setupDone = value;
	}

	--pendingTimestamps;
}

bool RenderSnapshot::hasPendingTimestamps() const
{
	return pendingTimestamps != 0;
}

size_t RenderSnapshot::getNumTimestamps() const
{
	return timestamps.size();
}

std::pair<uint64_t, uint64_t> RenderSnapshot::getFrameTimeRange() const
{
	return { startTime, endTime };
}

const RenderSnapshot::CommandTimeStamp& RenderSnapshot::getCommandTimeStamp(size_t idx) const
{
	static CommandTimeStamp dummy;

	if (idx >= timestamps.size()) {
		return dummy;
	}
	return timestamps.at(idx);
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

void RenderSnapshot::playDraw(Painter& painter, const DrawData& data, std::shared_ptr<const MaterialDefinition> debugMaterial) const
{
	auto material = data.material;
	if (debugMaterial) {
		const auto& srcPass = debugMaterial->getPass(0);

		material = material->clone();
		auto definition = std::make_shared<MaterialDefinition>(material->getDefinition());
		for (auto& pass: definition->getPasses()) {
			pass.replacePixelShader(srcPass, painter.video);
			pass.setBlend(srcPass.getBlend());
			pass.getDepthStencil() = srcPass.getDepthStencil();
		}
		material->setDefinition(std::move(definition));
	}
	painter.executeDrawPrimitives(*material, data.numVertices, data.vertexData, data.indices, data.primitive, data.allIndicesAreQuads);
}
