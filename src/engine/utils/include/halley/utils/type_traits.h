#pragma once

namespace Halley
{
	template<typename... Ts> struct make_void { typedef void type;};
	template<typename... Ts> using void_t = typename make_void<Ts...>::type;
}
