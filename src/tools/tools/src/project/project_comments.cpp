#include "halley/tools/project/project_comments.h"

#include "halley/data_structures/config_node.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
using namespace Halley;

ProjectComment::ProjectComment(const ConfigNode& node)
{
	pos = node["pos"].asVector2f({});
	text = node["text"].asString({});
	scene = node["scene"].asString({});
	priority = node["priority"].asEnum<ProjectCommentPriority>({});
}

ConfigNode ProjectComment::toConfigNode() const
{
	ConfigNode::MapType result;
	result["pos"] = pos;
	result["text"] = text;
	result["scene"] = scene;
	result["priority"] = priority;
	return result;
}

bool ProjectComment::operator==(const ProjectComment& other) const
{
	return pos == other.pos && text == other.text && priority == other.priority;
}

bool ProjectComment::operator!=(const ProjectComment& other) const
{
	return !(*this == other);
}

ProjectComments::ProjectComments(Path commentsRoot)
	: commentsRoot(std::move(commentsRoot))
	, monitor(commentsRoot)
{
	loadAll();
}

Vector<std::pair<UUID, const ProjectComment*>> ProjectComments::getComments(const String& scene) const
{
	Vector<std::pair<UUID, const ProjectComment*>> result;
	for (auto& [k, v]: comments) {
		if (v.scene == scene) {
			result.emplace_back(k, &v);
		}
	}
	return result;
}

const ProjectComment& ProjectComments::getComment(const UUID& id) const
{
	return comments.at(id);
}

UUID ProjectComments::addComment(ProjectComment comment)
{
	auto id = UUID::generate();
	setComment(id, std::move(comment));
	return id;
}

void ProjectComments::deleteComment(const UUID& id)
{
	deleteFile(id);
	comments.erase(id);
}

void ProjectComments::setComment(const UUID& id, ProjectComment comment)
{
	saveFile(id, comment);
	comments[id] = std::move(comment);
}

void ProjectComments::updateComment(const UUID& id, std::function<void(ProjectComment&)> f)
{
	auto& comment = comments[id];
	f(comment);
	saveFile(id, comment);
}

uint64_t ProjectComments::getVersion() const
{
	return version;
}

void ProjectComments::update(Time t)
{
	monitorTime += t;
	const auto threshold = monitor.hasRealImplementation() ? 0.5 : 2.0;

	if (monitorTime > threshold) {
		monitorTime = 0;
		if (monitor.poll()) {
			loadAll();
		}
	}
}

void ProjectComments::loadAll()
{
	bool modified = false;
	for (const auto& path: FileSystem::enumerateDirectory(commentsRoot)) {
		modified = loadFile(path) || modified;
	}

	if (modified) {
		++version;
		Logger::logDev("Comments loaded from disk");
	}
}

void ProjectComments::saveFile(const UUID& id, const ProjectComment& comment)
{
	YAMLConvert::EmitOptions options;
	Path::writeFile(getPath(id), YAMLConvert::generateYAML(comment.toConfigNode(), options));
	++version;
}

bool ProjectComments::loadFile(const Path& path)
{
	auto filename = path.getFilename().replaceExtension("").toString();
	if (filename.startsWith("comment_") && filename.length() == 44) {
		const auto uuid = UUID(filename.substr(8));
		if (uuid.isValid()) {
			const auto configFile = YAMLConvert::parseConfig(Path::readFile(path));
			auto comment = ProjectComment(configFile.getRoot());

			auto iter = comments.find(uuid);
			if (iter != comments.end()) {
				if (iter->second != comment) {
					iter->second = std::move(comment);
					return true;
				}
			} else {
				comments[uuid] = std::move(comment);
				return true;
			}
		}
	}
	return false;
}

void ProjectComments::deleteFile(const UUID& id)
{
	Path::removeFile(getPath(id));
	++version;
}

Path ProjectComments::getPath(const UUID& id) const
{
	return commentsRoot / ("comment_" + id.toString() + ".yaml");
}
