#include "halley/core/game/game_console.h"
#include "halley/text/halleystring.h"

using namespace Halley;

void GameConsole::registerConsoleListener(IGameConsoleListener* listener)
{
	listeners.insert(listener);
}

void GameConsole::removeConsoleListener(IGameConsoleListener* listener)
{
	listeners.erase(listener);
}

void GameConsole::registerConsoleCommand(const String& command, GameConsoleCallback callback)
{
	commands[command] = std::move(callback);
}

void GameConsole::removeConsoleCommand(const String& command)
{
	commands.erase(command);
}

String GameConsole::onHelp(std::vector<String> args)
{
	String result = "Commands available:";
	for (auto& c: commands) {
		result += "\n  " + c.first;
	}
	return result;
}

GameConsole::GameConsole()
{
	registerConsoleCommand("help", [=](std::vector<String> args) -> String { return onHelp(std::move(args)); });
}

void GameConsole::sendMessage(const String& string)
{
	for (auto& l: listeners) {
		l->onConsoleMessage(string);
	}
}

void GameConsole::sendCommand(const String& string)
{
	auto splitStr = string.split(' ');
	auto command = std::move(splitStr[0]);
	auto iter = commands.find(command);
	if (iter != commands.end()) {
		splitStr.erase(splitStr.begin());
		sendMessage(iter->second(splitStr));
	} else {
		sendMessage("Unknown command: " + command);
	}
}
