#pragma once

#include "ui_textinput.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class UIFactory;
	class UIStyle;

	class UIDebugConsoleResponse {
	public:
		UIDebugConsoleResponse();
		UIDebugConsoleResponse(String response, bool closeConsole = false);

		const String& getResponse() const;
		bool isCloseConsole() const;
		
	private:
		String response;
		bool closeConsole = false;
	};

	using UIDebugConsoleCallback = std::function<UIDebugConsoleResponse(std::vector<String>)>;

	class UIDebugConsoleSyntax {
	public:
		using Callback = std::function<std::vector<String>()>;
		
		struct Arg {
			String name;
			String type;
			Callback validOptionsCallback;

			Arg() = default;
			Arg(String name, String type) : name(std::move(name)), type(std::move(type)) {}
			Arg(String name, Callback validOptions) : name(std::move(name)), type("enum"), validOptionsCallback(std::move(validOptions)) {}

			bool checkArgument(const String& arg) const;
		};
		
		struct Variant {
			std::vector<Arg> args;

			Variant() = default;
			Variant(std::initializer_list<Arg> args) : args(std::move(args)) {}
			Variant(Arg arg) { args.emplace_back(std::move(arg)); }
			String getSyntax() const;
		};

		UIDebugConsoleSyntax() = default;
		UIDebugConsoleSyntax(std::initializer_list<Arg> args);
		UIDebugConsoleSyntax(std::initializer_list<Variant> variants);
		
		bool hasSyntax() const;
		std::optional<String> checkSyntax(const String& command, gsl::span<const String> args) const;
		std::vector<StringUTF32> getAutoComplete(const StringUTF32& line) const;

	private:
		std::vector<Variant> variants;

		struct VariantMatch {
			OptionalLite<size_t> variantN;
			size_t argN;
			size_t argStart;
			OptionalLite<size_t> invalidArg;
		};
		VariantMatch getVariantMatch(const String& line, bool validate) const;
		VariantMatch getVariantMatch(const String& command, gsl::span<const String> args, bool validate) const;
	};

	struct UIDebugConsoleCommandData {
		UIDebugConsoleCallback callback;
		ExecutionQueue* queue = nullptr;
		UIDebugConsoleSyntax syntax;
	};

	class UIDebugConsoleCommands {
	public:
		void addCommand(String command, UIDebugConsoleCallback callback);
		void addCommand(String command, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax);
		void addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback);
		void addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax);

		const std::map<String, UIDebugConsoleCommandData>& getCommands() const;

	private:
		std::map<String, UIDebugConsoleCommandData> commands;
	};

	class UIDebugConsoleController {
	public:
		UIDebugConsoleController();

		Future<UIDebugConsoleResponse> runCommand(String command, std::vector<String> args);
		String runHelp();
		
		void addCommands(UIDebugConsoleCommands& commands);
		void removeCommands(UIDebugConsoleCommands& commands);
		void clearCommands();

		std::vector<StringUTF32> getAutoComplete(const StringUTF32& line) const;

	private:
		std::vector<UIDebugConsoleCommands*> commands;
		std::unique_ptr<UIDebugConsoleCommands> baseCommandSet;
	};

    class UIDebugConsole : public UIWidget {
    public:
		UIDebugConsole(const String& id, UIFactory& factory, std::shared_ptr<UIDebugConsoleController> controller);
    	
		void show();
		void hide();
		void addLine(const String& line, Colour colour);

    	void setForcePaintMask(int mask);

    	const std::shared_ptr<UIDebugConsoleController>& getController() const;

		void onAddedToRoot(UIRoot& root) override;
    
	protected:
		void drawChildren(UIPainter& painter) const override;
    	
    private:
		void setup();
		void runCommand(const String& command);

		UIFactory& factory;
		std::shared_ptr<UIDebugConsoleController> controller;
		Future<String> pendingCommand;
		std::shared_ptr<UITextInput> inputField;
    	std::optional<int> forceMask;
    };
}
