#pragma once
#include "halley/core/entry/entry_point.h"
#include "halley/file/path.h"
#include "halley/tools/dll/dynamic_library.h"
#include "halley/core/game/game.h"

namespace Halley {
	class IProjectDLLListener;

	class ProjectDLL {
    public:
		enum class Status {
			Unloaded,
			Loaded,
			DLLNotFound,
			InvalidDLL,
			WrongDLLVersion,
			DLLCrash
		};
		
		ProjectDLL(const Path& path, const HalleyAPI& api);
    	virtual ~ProjectDLL();

		void load();
		void unload();
		bool isLoaded() const;
		
		void notifyReload();
		void reloadIfChanged();
		void addReloadListener(IProjectDLLListener& listener);
		void removeReloadListener(IProjectDLLListener& listener);

		Game& getGame() const;
		Status getStatus() const;

	private:
    	DynamicLibrary dll;
		const HalleyAPI& api;

		Path path;
		IHalleyEntryPoint* entryPoint = nullptr;
		std::unique_ptr<Game> game;

		Status status = Status::Unloaded;

		std::set<IProjectDLLListener*> reloadListeners;

		void setStatus(Status status);
	};

	class IProjectDLLListener {
	public:
		virtual ~IProjectDLLListener() = default;
		virtual void onProjectDLLStatusChange(ProjectDLL::Status status) = 0;
	};

	template <>
	struct EnumNames<ProjectDLL::Status> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"Unloaded",
				"Loaded",
				"DLLNotFound",
				"InvalidDLL",
				"WrongDLLVersion",
				"DLLCrash"
			}};
		}
	};
}
