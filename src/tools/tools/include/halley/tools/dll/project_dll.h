#pragma once
#include "dynamic_library.h"
#include "halley/core/entry/entry_point.h"
#include "halley/file/path.h"
#include "halley/core/game/game.h"

namespace Halley {
	class IProjectDLLListener;

	class ProjectDLL : private IDynamicLibraryListener {
    public:
		enum class Status {
			Unloaded,
			Loaded,
			DLLNotFound,
			InvalidDLL,
			DLLVersionTooLow,
			DLLVersionTooHigh,
			DLLCrash
		};
		
		ProjectDLL(const Path& path, const HalleyStatics& statics);
    	virtual ~ProjectDLL();

		void load();
		void unload();
		bool isLoaded() const;
		
		void reloadIfChanged();
		void addReloadListener(IProjectDLLListener& listener);
		void removeReloadListener(IProjectDLLListener& listener);

		Game& getGame() const;
		Status getStatus() const;

	private:
    	DynamicLibrary dll;
		const HalleyStatics& statics;

		Path path;
		IHalleyEntryPoint* entryPoint = nullptr;
		std::unique_ptr<Game> game;

		Status status = Status::Unloaded;

		std::set<IProjectDLLListener*> reloadListeners;

		void setStatus(Status status);

		void onLoadDLL() override;
		void onUnloadDLL() override;
	};

	class IProjectDLLListener {
	public:
		virtual ~IProjectDLLListener() = default;
		virtual void onProjectDLLStatusChange(ProjectDLL::Status status) = 0;
	};

	template <>
	struct EnumNames<ProjectDLL::Status> {
		constexpr std::array<const char*, 7> operator()() const {
			return{{
				"Unloaded",
				"Loaded",
				"DLLNotFound",
				"InvalidDLL",
				"DLLVersionTooLow",
				"DLLVersionTooHigh",
				"DLLCrash"
			}};
		}
	};
}
