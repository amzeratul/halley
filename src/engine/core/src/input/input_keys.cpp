#include "halley/input/input_keys.h"
#include "halley/support/logger.h"

using namespace Halley;

class KeyCodeNameMapping {
public:
	Vector<std::string_view> toString;
	Vector<std::string_view> toName;
	HashMap<std::string_view, KeyCode> toKeyCode;

	KeyCodeNameMapping()
	{
		initBindings();
	}

private:
	void add(KeyCode code, std::string_view str, std::string_view name = "")
	{
		toString[static_cast<size_t>(code)] = str;
		toName[static_cast<size_t>(code)] = name.empty() ? str : name;
		toKeyCode[str] = code;
	}

	void initBindings()
	{
		using enum KeyCode;

		toString.resize(static_cast<size_t>(Last) + 1, "");
		toName.resize(static_cast<size_t>(Last) + 1, "");

		add(Unknown, "Unknown");
		add(Enter, "Enter");
		add(Esc, "Esc");
		add(Backspace, "Backspace");
		add(Tab, "Tab");
		add(Space, "Space");
		add(A, "A");
		add(B, "B");
		add(C, "C");
		add(D, "D");
		add(E, "E");
		add(F, "F");
		add(G, "G");
		add(H, "H");
		add(I, "I");
		add(J, "J");
		add(K, "K");
		add(L, "L");
		add(M, "M");
		add(N, "N");
		add(O, "O");
		add(P, "P");
		add(Q, "Q");
		add(R, "R");
		add(S, "S");
		add(T, "T");
		add(U, "U");
		add(V, "V");
		add(W, "W");
		add(X, "X");
		add(Y, "Y");
		add(Z, "Z");
		add(Num0, "Num0", "0");
		add(Num1, "Num1", "1");
		add(Num2, "Num2", "2");
		add(Num3, "Num3", "3");
		add(Num4, "Num4", "4");
		add(Num5, "Num5", "5");
		add(Num6, "Num6", "6");
		add(Num7, "Num7", "7");
		add(Num8, "Num8", "8");
		add(Num9, "Num9", "9");
		add(Minus, "Minus", "-");
		add(Equals, "Equals", "=");
		add(LeftBracket, "LeftBracket", "[");
		add(RightBracket, "RightBracket", "]");
		add(Backslash, "Backslash", "\\");
		add(Semicolon, "Semicolon", ";");
		add(Apostrophe, "Apostrophe", "'");
		add(Grave, "Grave", "`");
		add(Caret, "Caret", "^");
		add(Comma, "Comma", ",");
		add(Period, "Period", ".");
		add(Slash, "Slash", "/");
		add(CapsLock, "CapsLock");
		add(F1, "F1");
		add(F2, "F2");
		add(F3, "F3");
		add(F4, "F4");
		add(F5, "F5");
		add(F6, "F6");
		add(F7, "F7");
		add(F8, "F8");
		add(F9, "F9");
		add(F10, "F10");
		add(F11, "F11");
		add(F12, "F12");
		add(PrintScreen, "PrintScreen");
		add(ScrollLock, "ScrollLock");
		add(Pause, "Pause");
		add(Insert, "Insert", "Ins");
		add(Home, "Home");
		add(PageUp, "PageUp", "PgUp");
		add(Delete, "Delete", "Del");
		add(End, "End");
		add(PageDown, "PageDown", "PgDn");
		add(Right, "Right");
		add(Left, "Left");
		add(Down, "Down");
		add(Up, "Up");
		add(NumLock, "NumLock");
		add(KeypadDivide, "KeypadDivide", "KP/");
		add(KeypadMultiply, "KeypadMultiply", "KP*");
		add(KeypadMinus, "KeypadMinus", "KP-");
		add(KeypadPlus, "KeypadPlus", "KP+");
		add(KeypadEnter, "KeypadEnter", "KPEnter");
		add(Keypad1, "Keypad1", "KP1");
		add(Keypad2, "Keypad2", "KP2");
		add(Keypad3, "Keypad3", "KP3");
		add(Keypad4, "Keypad4", "KP4");
		add(Keypad5, "Keypad5", "KP5");
		add(Keypad6, "Keypad6", "KP6");
		add(Keypad7, "Keypad7", "KP7");
		add(Keypad8, "Keypad8", "KP8");
		add(Keypad9, "Keypad9", "KP9");
		add(Keypad0, "Keypad0", "KP0");
		add(KeypadPeriod, "KeypadPeriod", "KP.");
		add(KeypadEquals, "KeypadEquals", "KP=");
		add(KeypadComma, "KeypadComma", "KP,");
		add(VolumeUp, "VolumeUp");
		add(VolumeDown, "VolumeDown");
		add(LCtrl, "LCtrl");
		add(LShift, "LShift");
		add(LAlt, "LAlt");
		add(LMod, "LMod");
		add(RCtrl, "RCtrl");
		add(RShift, "RShift");
		add(RAlt, "RAlt");
		add(RMod, "RMod");
		add(Mode, "Mode");
		add(AudioNext, "AudioNext");
		add(AudioPrev, "AudioPrev");
		add(AudioStop, "AudioStop");
		add(AudioPlay, "AudioPlay");
		add(AudioMute, "AudioMute");
		add(AndroidMenu, "AndroidMenu");
		add(AndroidBack, "AndroidBack");
		add(Last, "Last");
	}
};

namespace {
	const KeyCodeNameMapping& getMapping()
	{
		static KeyCodeNameMapping mapping;
		return mapping;
	}
}


String KeyCodes::toString(KeyCode code, KeyMods mods)
{
	return getMapping().toString.at(static_cast<size_t>(code));
}

String KeyCodes::toName(KeyCode code)
{
	return getMapping().toName.at(static_cast<size_t>(code));
}

String KeyCodes::toName(KeyMods mods)
{
	if (mods == KeyMods::Ctrl) {
		return "Ctrl";
	} else if (mods == KeyMods::Alt) {
		return "Alt";
	} else if (mods == KeyMods::Shift) {
		return "Shift";
	} else if (mods == KeyMods::Mod) {
		return "Mod";
	} else {
		Logger::logError("Cannot convert key mods to name: " + Halley::toString(int(mods)));
		return "?";
	}
}

std::optional<String> KeyCodes::tryToName(KeyCode code)
{
	const auto& toName = getMapping().toName;
	const auto idx = static_cast<size_t>(code);
	if (idx >= toName.size() || toName[idx].empty()) {
		return std::nullopt;
	}
	return toName[idx];
}

KeyCode KeyCodes::fromString(const String& str)
{
	if (auto result = tryFromString(str)) {
		return *result;
	} else {
		Logger::logError("Unable to convert \"" + str + "\" to keycode.");
		return KeyCode::Unknown;
	}
}

std::optional<KeyCode> KeyCodes::tryFromString(const String& str)
{
	const auto& toKeyCode = getMapping().toKeyCode;
	if (const auto iter = toKeyCode.find(str); iter != toKeyCode.end()) {
		return iter->second;
	}
	return std::nullopt;
}

std::pair<KeyCode, KeyMods> KeyCodes::fromStringWithMods(const String& str)
{
	KeyMods mods = KeyMods::None;
	KeyCode code = KeyCode::Unknown;

	const auto split = String(str).split('+');
	for (size_t i = 0 ; i < split.size(); ++i) {
		if (i == split.size() - 1) {
			// Last one is key code
			code = fromString(split[i]);
		} else {
			if (split[i] == "Ctrl") {
				mods = mods | KeyMods::Ctrl;
			} else if (split[i] == "Shift") {
				mods = mods | KeyMods::Ctrl;
			} else if (split[i] == "Alt") {
				mods = mods | KeyMods::Ctrl;
			} if (split[i] == "Mod") {
				mods = mods | KeyMods::Ctrl;
			}
		}
	}

	return { code, mods };
}
