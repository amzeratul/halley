#include "audio_layers_editor.h"

#include "audio_expression_editor.h"
#include "audio_object_editor.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"

using namespace Halley;

AudioLayersEditor::AudioLayersEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectLayers& layers)
	: UIWidget("audio_layers_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, layers(layers)
{
	factory.loadUI(*this, "halley/audio_editor/audio_layers_editor");
}

void AudioLayersEditor::onMakeUI()
{
	const auto layerList = getWidgetAs<UIList>("layers");

	for (size_t i = 0; i < layers.getLayers().size(); ++i) {
		layerList->addItem(toString(i), std::make_shared<AudioLayersEditorLayer>(factory, *this, i), 1);
	}

	setHandle(UIEventType::ListItemsSwapped, [=] (const UIEvent& event)
	{
		std::swap(layers.getLayers()[event.getIntData()], layers.getLayers()[event.getIntData2()]);
		editor.markModified(true);
	});
}

AudioSubObjectLayers::Layer& AudioLayersEditor::getLayer(size_t idx)
{
	return layers.getLayers()[idx];
}

void AudioLayersEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioLayersEditorLayer::AudioLayersEditorLayer(UIFactory& factory, AudioLayersEditor& layersEditor, size_t idx)
	: UIWidget("audio_layers_editor_layer", Vector2f(), UISizer())
	, factory(factory)
	, layersEditor(layersEditor)
	, idx(idx)
{
	factory.loadUI(*this, "halley/audio_editor/audio_layers_editor_layer");
}

void AudioLayersEditorLayer::onMakeUI()
{
	auto& layer = layersEditor.getLayer(idx);
	const auto& name = layer.object->getName();
	getWidgetAs<UILabel>("layerName")->setText(LocalisedString::fromUserString(name));

	getWidget("expressionContainer")->add(std::make_shared<AudioExpressionEditor>(factory, layer.expression), 1);

	bindData("synchronised", layer.synchronised, [this, &layer] (bool value)
	{
		layer.synchronised = value;
		layersEditor.markModified(idx);
	});
}
