#pragma once
#include <memory>
#include <vector>

#include "render_graph_pin_type.h"
#include "halley/core/graphics/texture_descriptor.h"

namespace Halley {
	class RenderGraphDefinition;
	class Camera;
	class TextureRenderTarget;
	class Texture;
	class RenderTarget;
	class VideoAPI;
	class RenderGraph;
	class RenderContext;
	class Painter;
	class Material;
	class RenderGraphNode;
	
	class RenderGraph {
	public:
		using PaintMethod = std::function<void(Painter&)>;

		RenderGraph();
		explicit RenderGraph(std::shared_ptr<const RenderGraphDefinition> graphDefinition);

		void update();
		void render(const RenderContext& rc, VideoAPI& video);

		const Camera* tryGetCamera(std::string_view id) const;
		void setCamera(std::string_view id, const Camera& camera);

		const PaintMethod* tryGetPaintMethod(std::string_view id) const;
		void setPaintMethod(std::string_view id, PaintMethod method);

		void applyVariable(Material& material, const String& name, const ConfigNode& value) const;
		void setVariable(std::string_view name, float value);
		void setVariable(std::string_view name, Vector2f value);
		void setVariable(std::string_view name, Vector3f value);
		void setVariable(std::string_view name, Vector4f value);
		void setVariable(std::string_view name, Colour4f value);

		bool remapNode(std::string_view outputName, uint8_t outputPin, std::string_view inputName, uint8_t inputPin);
		void resetGraph();

	private:
		enum class VariableType {
			None,
			Float,
			Float2,
			Float3,
			Float4
		};
		struct Variable {
			Vector4f var;
			VariableType type = VariableType::None;

			void apply(Material& material, const String& name) const;
			Variable& operator=(float v);
			Variable& operator=(Vector2f v);
			Variable& operator=(Vector3f v);
			Variable& operator=(Vector4f v);
			Variable& operator=(Colour4f v);
		};
		
		std::vector<std::unique_ptr<RenderGraphNode>> nodes;
		std::map<String, RenderGraphNode*> nodeMap;
		
		std::map<String, Camera> cameras;
		std::map<String, PaintMethod> paintMethods;
		std::map<String, Variable> variables;

		std::shared_ptr<const RenderGraphDefinition> graphDefinition;
		int lastDefinitionVersion = 0;

		void addNode(String id, std::unique_ptr<RenderGraphNode> node);
		void addOutputNode();
		RenderGraphNode* getNode(const String& id);
		RenderGraphNode* tryGetNode(const String& id);

		void loadDefinition(std::shared_ptr<const RenderGraphDefinition> definition);
	};
}
