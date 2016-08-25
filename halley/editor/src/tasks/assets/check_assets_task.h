#pragma once
#include "../editor_task.h"

namespace Halley
{
	class CheckAssetsTask : public EditorTask
	{
	public:
		CheckAssetsTask(Vector<String>&& srcFolders, String dstFolder);

	protected:
		void run() override;

	private:
		Vector<String> srcFolders;
		String dstFolder;
	};
}
