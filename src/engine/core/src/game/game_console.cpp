#include "halley/core/game/game_console.h"
#include "halley/text/halleystring.h"

void Halley::GameConsole::registerConsoleListener(IGameConsoleListener* listener)
{
	listeners.insert(listener);
}

void Halley::GameConsole::removeConsoleListener(IGameConsoleListener* listener)
{
	listeners.erase(listener);
}

void Halley::GameConsole::registerConsoleCommand(const String& command, GameConsoleCallback callback)
{
	commands[command] = std::move(callback);
}

void Halley::GameConsole::removeConsoleCommand(const String& command)
{
	commands.erase(command);
}

void Halley::GameConsole::sendMessage(const String& string)
{
	for (auto& l: listeners) {
		l->onConsoleMessage(string);
	}
}

void Halley::GameConsole::sendCommand(const String& string)
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
