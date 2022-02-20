#pragma once
#include <gsl/span>
#include "halley/data_structures/vector.h"
#include "component_schema.h"
#include "custom_type_schema.h"
#include "halley/data_structures/hash_map.h"
#include "halley/text/halleystring.h"
#include "message_schema.h"
#include "system_message_schema.h"
#include "system_schema.h"

namespace YAML
{
	class Node;
}

namespace Halley {
	struct CodegenSourceInfo {
		String filename;
		gsl::span<const gsl::byte> data;
		bool generate = false;
	};

	class Codegen;
	
    class ECSData {
    public:
		void loadSources(Vector<CodegenSourceInfo> files);

		const HashMap<String, ComponentSchema>& getComponents() const;
		const HashMap<String, SystemSchema>& getSystems() const;
		const HashMap<String, MessageSchema>& getMessages() const;
		const HashMap<String, SystemMessageSchema>& getSystemMessages() const;
		const HashMap<String, CustomTypeSchema>& getCustomTypes() const;

    	void clear();
		int getRevision() const;

	private:
    	void addSource(CodegenSourceInfo sourceInfo);
		void addComponent(YAML::Node rootNode, bool generate);
		void addSystem(YAML::Node rootNode, bool generate);
		void addMessage(YAML::Node rootNode, bool generate);
    	void addSystemMessage(YAML::Node rootNode, bool generate);
		void addType(YAML::Node rootNode);
		String getInclude(String typeName) const;

    	void validate();
		void process();

    	HashMap<String, ComponentSchema> components;
		HashMap<String, SystemSchema> systems;
		HashMap<String, MessageSchema> messages;
    	HashMap<String, SystemMessageSchema> systemMessages;
		HashMap<String, CustomTypeSchema> types;
		int revision = 0;
    };
}
