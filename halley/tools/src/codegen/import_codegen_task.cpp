#include "halley/tools/codegen/import_codegen_task.h"
#include "halley/tools/codegen/codegen.h"

using namespace Halley;

ImportCodegenTask::ImportCodegenTask(ImportAssetsDatabase& db, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& _files)
	: EditorTask("Generating code", true, true)
	, db(db)
	, files(_files)
	, srcPath(files.at(0).srcDir)
	, dstPath(assetsPath)
{	
}

void ImportCodegenTask::run()
{
	Codegen codegen;

	setProgress(0.0f, "Loading sources");
	codegen.loadSources(srcPath);
	if (isCancelled()) {
		return;
	}

	setProgress(0.25f, "Validating");
	codegen.validate();
	if (isCancelled()) {
		return;
	}

	setProgress(0.50f, "Processing");
	codegen.process();
	if (isCancelled()) {
		return;
	}

	setProgress(0.75f, "Generating code");
	codegen.generateCode(dstPath);

	setProgress(1.0f);

	for (auto& f: files) {
		db.markAsImported(f);
	}
	db.save();
}
