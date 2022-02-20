#pragma once

#include <set>
#include <functional>
#include "halley/data_structures/vector.h"
#include <map>

namespace Halley {
	class String;

	class IGameConsoleListener {
	public:
		virtual ~IGameConsoleListener() = default;

		virtual void onConsoleMessage(const String& string) = 0;
	};

	class GameConsole;
	using GameConsoleCallback = std::function<String(Vector<String>)>;

    class GameConsole {
    public:
		GameConsole();

		void sendMessage(const String& string);
	    void sendCommand(const String& string);

    	void registerConsoleListener(IGameConsoleListener* listener);
		void removeConsoleListener(IGameConsoleListener* listener);

		void registerConsoleCommand(const String& command, GameConsoleCallback callback);
		void removeConsoleCommand(const String& command);

    private:
		std::set<IGameConsoleListener*> listeners;
		std::map<String, GameConsoleCallback> commands;

		String onHelp(Vector<String> args);
    };
}
