#pragma once

#include "dynamic_grid.h"
#include "halley/maths/rect.h"
#include "vector.h"
#include "hash_map.h"

namespace Halley {
	class RectangleSpatialChecker {
	public:
		typedef int DataType;

		struct QueryResults {
			size_t n;
			DataType* results;
		};

		// Resolution is the grid size, given in 2^resolution units.
		// Lower values = finer resolution.
		// Recommended: a value of around 7 (128x128)
		RectangleSpatialChecker(int resolution);

		bool add(Rect4i rect, DataType data);
		bool remove(DataType data);
		bool update(Rect4i rect, DataType data);

		QueryResults query(Rect4i rect);

	private:
		struct Entry {
			Rect4i rect;
			DataType data;
			int hashMask;
		};

		HashMap<DataType, Entry> entries;
		Vector<DataType> resultsBuffer;
		int sweep = 0;

		int resolution;

		typedef std::vector<Entry> GridCell;
		DynamicGrid<GridCell> grid;

		void resetSweep();
		Vector2i pointToCell(Vector2i point) const;
		GridCell& getGridCell(int x, int y);

		bool updateData(Entry& entry, Rect4i prev, Rect4i next);
	};
}
