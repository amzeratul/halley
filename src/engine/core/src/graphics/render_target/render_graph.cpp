#include "graphics/render_target/render_graph.h"
#include "api/video_api.h"
#include "graphics/render_context.h"
#include "graphics/material/material.h"
#include "graphics/render_target/render_graph_definition.h"
#include "graphics/render_target/render_graph_node.h"
#include "graphics/render_target/render_target.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;


RenderGraph::RenderGraph()
{
	addOutputNode();
}

RenderGraph::RenderGraph(std::shared_ptr<const RenderGraphDefinition> def)
{
	loadDefinition(std::move(def));
}

void RenderGraph::loadDefinition(std::shared_ptr<const RenderGraphDefinition> definition)
{
	nodes.clear();
	nodeMap.clear();
	
	graphDefinition = std::move(definition);
	lastDefinitionVersion = graphDefinition->getAssetVersion();
	
	addOutputNode();
	
	for (const auto& nodeDefinition: graphDefinition->getNodes()) {
		addNode(nodeDefinition.id, std::make_unique<RenderGraphNode>(nodeDefinition));
	}
	for (const auto& connectionDefinition: graphDefinition->getConnections()) {
		auto* from = getNode(connectionDefinition.fromId);
		auto* to = getNode(connectionDefinition.toId);

		to->connectInput(connectionDefinition.toPin, *from, connectionDefinition.fromPin);
	}
}

void RenderGraph::update()
{
	// Hot-reload
	if (graphDefinition && graphDefinition->getAssetVersion() != lastDefinitionVersion) {
		loadDefinition(graphDefinition);
	}
}

void RenderGraph::addNode(String id, std::unique_ptr<RenderGraphNode> node)
{
	if (nodeMap.find(id) != nodeMap.end()) {
		throw Exception("Duplicate id \"" + id + "\" in RenderGraph.", HalleyExceptions::Graphics);
	}
	
	nodes.emplace_back(std::move(node));
	nodeMap[std::move(id)] = nodes.back().get();
}

void RenderGraph::addOutputNode()
{
	RenderGraphDefinition::Node nodeDef;
	nodeDef.method = RenderGraphMethod::Output;
	addNode("output", std::make_unique<RenderGraphNode>(nodeDef));
}

RenderGraphNode* RenderGraph::getNode(const String& id)
{
	return nodeMap.at(id);
}

void RenderGraph::render(const RenderContext& rc, VideoAPI& video)
{
	update();
	
	for (auto& node: nodes) {
		node->startRender();
	}

	const auto renderSize = rc.getDefaultRenderTarget().getViewPort().getSize();
	auto* outputNode = getNode("output");
	outputNode->prepareDependencyGraph(video, renderSize);

	for (auto& node: nodes) {
		node->determineIfNeedsRenderTarget();
	}
	for (auto& node: nodes) {
		node->allocateVideoResources(video);
	}

	std::vector<RenderGraphNode*> renderQueue;
	renderQueue.reserve(nodes.size());
	for (auto& node: nodes) {
		if (node->activeInCurrentPass && node->depsLeft == 0) {
			renderQueue.push_back(node.get());
		}
	}

	for (size_t i = 0; i < renderQueue.size(); ++i) {
		renderQueue[i]->render(*this, rc, renderQueue);
	}
}

const Camera* RenderGraph::tryGetCamera(std::string_view id) const
{
	const auto iter = cameras.find(id);
	if (iter != cameras.end()) {
		return &iter->second;
	} else {
		return nullptr;
	}
}

void RenderGraph::setCamera(std::string_view id, const Camera& camera)
{
	cameras[id] = camera;
}

const RenderGraph::PaintMethod* RenderGraph::tryGetPaintMethod(std::string_view id) const
{
	const auto iter = paintMethods.find(id);
	if (iter != paintMethods.end()) {
		return &iter->second;
	} else {
		return nullptr;
	}
}

void RenderGraph::setPaintMethod(std::string_view id, PaintMethod method)
{
	paintMethods[id] = std::move(method);
}

void RenderGraph::applyVariable(Material& material, const String& name, const ConfigNode& value) const
{
	if (value.getType() == ConfigNodeType::String) {
		variables.at(value.asString()).apply(material, name);
	} else if (value.getType() == ConfigNodeType::Float || value.getType() == ConfigNodeType::Int) {
		material.set(name, value.asFloat());
	}
}

void RenderGraph::setVariable(std::string_view name, float value)
{
	variables[name] = value;
}

void RenderGraph::setVariable(std::string_view name, Vector2f value)
{
	variables[name] = value;
}

void RenderGraph::setVariable(std::string_view name, Vector3f value)
{
	variables[name] = value;
}

void RenderGraph::setVariable(std::string_view name, Vector4f value)
{
	variables[name] = value;
}

void RenderGraph::setVariable(std::string_view name, Colour4f value)
{
	variables[name] = value;
}

void RenderGraph::Variable::apply(Material& material, const String& name) const
{
	switch (type) {
	case VariableType::Float:
		material.set(name, var.x);
		break;
	case VariableType::Float2:
		material.set(name, var.xy());
		break;
	case VariableType::Float3:
		material.set(name, var.xyz());
		break;
	case VariableType::Float4:
		material.set(name, var);
		break;
	}
}

RenderGraph::Variable& RenderGraph::Variable::operator=(float v)
{
	var = Vector4f(v, 0, 0, 0);
	type = VariableType::Float;
	return *this;
}

RenderGraph::Variable& RenderGraph::Variable::operator=(Vector2f v)
{
	var = Vector4f(v.x, v.y, 0, 0);
	type = VariableType::Float2;
	return *this;
}

RenderGraph::Variable& RenderGraph::Variable::operator=(Vector3f v)
{
	var = Vector4f(v.x, v.y, v.z, 0);
	type = VariableType::Float3;
	return *this;
}

RenderGraph::Variable& RenderGraph::Variable::operator=(Vector4f v)
{
	var = v;
	type = VariableType::Float4;
	return *this;
}

RenderGraph::Variable& RenderGraph::Variable::operator=(Colour4f v)
{
	var = Vector4f(v.r, v.g, v.b, v.a);
	type = VariableType::Float4;
	return *this;
}
