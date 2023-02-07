#pragma once
#include "halley/data_structures/hash_map.h"
#include "halley/file/directory_monitor.h"
#include "halley/file/path.h"
#include "halley/maths/uuid.h"
#include "halley/maths/vector2.h"
#include "halley/time/halleytime.h"

namespace Halley {
    enum class ProjectCommentPriority {
	    Low,
        Medium,
        High
    };

	template <>
	struct EnumNames<ProjectCommentPriority> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"low",
                "medium",
                "high"
			}};
		}
	};

    class ProjectComment {
    public:
        Vector2f pos;
        String text;
        String scene;
        ProjectCommentPriority priority = ProjectCommentPriority::Low;

        explicit ProjectComment(Vector2f pos = {}, String scene = "");
        ProjectComment(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        bool operator==(const ProjectComment& other) const;
        bool operator!=(const ProjectComment& other) const;
    };

    class ProjectComments {
    public:
        ProjectComments(Path commentsRoot);

        Vector<std::pair<UUID, const ProjectComment*>> getComments(const String& scene) const;
        const ProjectComment& getComment(const UUID& id) const;

        UUID addComment(ProjectComment comment);
        void deleteComment(const UUID& id);
        void setComment(const UUID& id, ProjectComment comment);
        void updateComment(const UUID& id, std::function<void(ProjectComment&)> f, bool immediate = false);

        uint64_t getVersion() const;
        void update(Time t);

    private:
        Path commentsRoot;
        DirectoryMonitor monitor;   // Important, this needs to be after commentsRoot, as it has dependent initialization
        Time monitorTime = 0;
        Time saveTimeout = 0;

        HashMap<UUID, ProjectComment> comments;
        uint64_t version = 0;
        Vector<UUID> toSave;
        HashMap<UUID, uint64_t> lastWrittenHash;

        void loadAll();
        void savePending();
        void saveFile(const UUID& id, const ProjectComment& comment);
        bool loadFile(const UUID& id, const Path& path);
        void deleteFile(const UUID& id);

        Path getPath(const UUID& id) const;
        UUID getUUID(const Path& path) const;
    };
}
