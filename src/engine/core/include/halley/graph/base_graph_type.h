#pragma once

#include "base_graph_enums.h"
#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/graphics/text/text_renderer.h"

namespace Halley {
	class World;
	class BaseGraph;
	class BaseGraphNode;

	class IGraphNodeType {
	public:
		struct SettingType {
			String name;
			String type;
			Vector<String> defaultValue;
		};

		constexpr static Colour4f parameterColour = Colour4f(0.47f, 0.58f, 0.98f);
		constexpr static Colour4f settingColour = Colour4f(0.97f, 0.35f, 0.35f);
		using PinType = GraphNodePinType;

		virtual ~IGraphNodeType() = default;

		virtual String getId() const = 0;
		virtual String getName() const = 0;
		virtual String getIconName(const BaseGraphNode& node) const;
		virtual String getLabel(const BaseGraphNode& node) const;
		virtual Colour4f getColour() const;

		virtual Vector<SettingType> getSettingTypes() const;
		virtual void updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const;

		virtual gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const = 0;
        PinType getPin(const BaseGraphNode& node, size_t n) const;

		virtual std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const;
		virtual String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const;
		virtual String getPinTypeName(PinType type) const;
		virtual std::pair<String, Vector<ColourOverride>> getDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx, const BaseGraph& graph) const;

		virtual bool canAdd() const { return true; }
        virtual bool canDelete() const { return true; }

		virtual std::optional<Vector2f> getNodeSize(const BaseGraphNode& node, float curZoom) const { return std::nullopt; }
		virtual int getSortOrder() const { return 0; }
	};

	class GraphNodeTypeCollection {
	public:
    	void addNodeType(std::unique_ptr<IGraphNodeType> nodeType);

		const IGraphNodeType* tryGetGraphNodeType(const String& typeId) const;
		Vector<String> getTypes(bool includeNonAddable) const;
		Vector<String> getNames(bool includeNonAddable) const;

	private:
    	std::map<String, std::unique_ptr<IGraphNodeType>> nodeTypes;
	};
}
