#pragma once
#include "../editor_task.h"

namespace Halley
{
	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(Vector<String>&& files, String destinationFolder);

	protected:
		void run() override;

	private:
		Vector<String> files;
		String destinationFolder;
	};
}
