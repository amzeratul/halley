/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

namespace Halley {
	enum class KeyCode {
		Unknown = 0,

		Enter = '\r',
		Esc = 0x1B,
		Backspace = '\b',
		Tab = '\t',
		Space = ' ',
		
		A = 'A',
		B = 'B',
		C = 'C',
		D = 'D',
		E = 'E',
		F = 'F',
		G = 'G',
		H = 'H',
		I = 'I',
		J = 'J',
		K = 'K',
		L = 'L',
		M = 'M',
		N = 'N',
		O = 'O',
		P = 'P',
		Q = 'Q',
		R = 'R',
		S = 'S',
		T = 'T',
		U = 'U',
		V = 'V',
		W = 'W',
		X = 'X',
		Y = 'Y',
		Z = 'Z',

		Num0 = '0',
		Num1 = '1',
		Num2 = '2',
		Num3 = '3',
		Num4 = '4',
		Num5 = '5',
		Num6 = '6',
		Num7 = '7',
		Num8 = '8',
		Num9 = '9',

		Minus = '-',
		Equals = '=',
		LeftBracket = '[',
		RightBracket = ']',
		Backslash = '\\',
		Semicolon = ';',
		Apostrophe = '\'',
		Grave = '`',
		Comma = ',',
		Period = '.',
		Slash = '/',

		CapsLock = 185,

		F1 = 186,
		F2 = 187,
		F3 = 188,
		F4 = 189,
		F5 = 190,
		F6 = 191,
		F7 = 192,
		F8 = 193,
		F9 = 194,
		F10 = 195,
		F11 = 196,
		F12 = 197,

		PrintScreen = 198,
		ScrollLock = 199,
		Pause = 200,

		Insert = 201,
		Home = 202,
		PageUp = 203,
		Delete = 204,
		End = 205,
		PageDown = 206,

		Right = 207,
		Left = 208,
		Down = 209,
		Up = 210,

		NumLock = 211,
		KeypadDivide = 212,
		KeypadMultiply = 213,
		KeypadMinus = 214,
		KeypadPlus = 215,
		KeypadEnter = 216,
		Keypad1 = 217,
		Keypad2 = 218,
		Keypad3 = 219,
		Keypad4 = 220,
		Keypad5 = 221,
		Keypad6 = 222,
		Keypad7 = 223,
		Keypad8 = 224,
		Keypad9 = 225,
		Keypad0 = 226,
		KeypadPeriod = 227,
		KeypadEquals = 231,
		KeypadComma = 261,

		VolumeUp = 256,
		VolumeDown = 257,

		LCtrl = 352,
		LShift = 353,
		LAlt = 354,
		LMod = 355,
		RCtrl = 356,
		RShift = 357,
		RAlt = 358,
		RMod = 359,

		Mode = 385,

		AudioNext = 386,
		AudioPrev = 387,
		AudioStop = 388,
		AudioPlay = 389,
		AudioMute = 390,

		Android_Menu = 246,
		Android_Back = 398,

		Last = 400
	};

	using Keys [[deprecated]] = KeyCode;

	enum class KeyMods {
		None = 0,
		Shift = 1,
		Ctrl = 2,
		CtrlShift = Ctrl | Shift,
		Alt = 4,
		ShiftAlt = Shift | Alt,
		CtrlAlt = Ctrl | Alt,
		CtrlShiftAlt = Ctrl | Shift | Alt,
		Mod = 8,
		ShiftMod = Shift | Mod,
		CtrlMod = Ctrl | Mod,
		CtrlShiftMod = Ctrl | Shift | Mod,
		AltMod = Alt | Mod,
		ShiftAltMod = Shift | Alt | Mod,
		CtrlAltMod = Ctrl | Alt | Mod,
		CtrlShiftAltMod = Ctrl | Shift | Alt | Mod,
	};
}
