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
        ProjectCommentPriority priority;

        ProjectComment() = default;
        ProjectComment(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        bool operator==(const ProjectComment& other) const;
        bool operator!=(const ProjectComment& other) const;
    };

    class ProjectComments {
    public:
        ProjectComments(Path commentsRoot);

        const HashMap<UUID, ProjectComment>& getComments() const;
        const ProjectComment& getComment(const UUID& id) const;

        UUID addComment(ProjectComment comment);
        void deleteComment(const UUID& id);
        void setComment(const UUID& id, ProjectComment comment);

        uint64_t getVersion() const;
        void update(Time t);

    private:
        Path commentsRoot;
        DirectoryMonitor monitor;   // Important, this needs to be after commentsRoot, as it has dependent initialization

        HashMap<UUID, ProjectComment> comments;
        uint64_t version = 0;

        void loadAll();
        void saveFile(const UUID& id, const ProjectComment& comment);
        bool loadFile(const Path& path);
        void deleteFile(const UUID& id);

        Path getPath(const UUID& id) const;
    };
}
