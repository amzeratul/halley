#pragma once
#include "halley/lua/lua_state.h"
#include "halley/scripting/script_environment.h"
#include "halley/entity/service.h"
#include "halley/entity/system_interface.h"

namespace Halley {

	class ScriptingService : public Service, public ILuaInterface {
	public:
		ScriptingService(std::unique_ptr<ScriptEnvironment> environment, Resources& resources, const String& initialLuaModule = "", bool devMode = false);
		~ScriptingService();

		ScriptEnvironment& getEnvironment() const;

		[[maybe_unused]] ConfigNode evaluateExpression(const String& expression, bool useResultCache = false, bool throwOnError = true) const;
		[[maybe_unused]] ConfigNode evaluateExpression(const LuaExpression& expression, bool useResultCache = false, bool throwOnError = true) const;
		[[nodiscard]] bool evaluateBoolExpression(const String& expression, bool onEmpty = true, bool useResultCache = false, bool throwOnError = true) const;
		[[nodiscard]] bool evaluateBoolExpression(const LuaExpression& expression, bool onEmpty = true, bool useResultCache = false, bool throwOnError = true) const;
		void clearResultCache();

		template <typename T>
		void setLuaGlobal(const String& key, const T& value)
		{
			// If the value already exists in cache, only store it if it's different. This is due to performance reasons.
			// This assumes this isn't modified in Lua itself; if it is, we'll have other issues (globals is used to clone Lua contexts)
			if (cacheGlobalValue(key, value)) {
				auto stack = LuaStackOps(*luaState);
				stack.push(value);
				stack.makeGlobal(key);
			}
		}

		template <typename T, typename R, typename... Ps>
		void setLuaGlobalCallback(const String& key, T* obj, R (T::*f)(Ps...))
		{
			auto stack = LuaStackOps(*luaState);
			stack.push(LuaCallbackBind(obj, f));
			stack.makeGlobal(key);
		}

		template <typename T, typename R, typename... Ps>
		void setLuaGlobalCallback(const String& key, const T* obj, R (T::*f)(Ps...) const)
		{
			auto stack = LuaStackOps(*luaState);
			stack.push(LuaCallbackBind(obj, f));
			stack.makeGlobal(key);
		}

		ConfigNode getLuaGlobal(const String& key);
		void copyLuaGlobal(const String& key, ScriptingService& source);

		LuaState& getLuaState() const override;
		LuaReference& getLuaReference(const LuaExpression& luaExpression) const override;

		std::function<std::shared_ptr<ScriptingService>()> deferredClone(std::unique_ptr<ScriptEnvironment> environment = {}) const;
		std::shared_ptr<ScriptingService> clone(std::unique_ptr<ScriptEnvironment> environment = {}) const;

	private:
		std::unique_ptr<ScriptEnvironment> scriptEnvironment;
		std::unique_ptr<LuaState> luaState;
		HashMap<String, ConfigNode> globals;
		String initialModule;
		bool devMode;
		Resources& resources;

	    mutable HashMap<String, ConfigNode> resultCache;
		mutable HashMap<String, std::unique_ptr<LuaReference>> luaReferences;

		template <typename T>
		bool cacheGlobalValue(const String& key, const T& value)
		{
			const auto iter = globals.find(key);

			ConfigNode result;

			if constexpr (std::is_same_v<T, ConfigNode>) {
				// This path avoids a copy if already in cache, which can be common and expensive
				if (iter != globals.end() && iter->second == value) {
					return false;
				}
				result = ConfigNode(value);
			} else {
				result = ConfigNode(value);
				if (iter != globals.end() && iter->second == result) {
					return false;
				}
			}
			globals[key] = std::move(result);
			return true;
		}
	};
}

using ScriptingService = Halley::ScriptingService;
