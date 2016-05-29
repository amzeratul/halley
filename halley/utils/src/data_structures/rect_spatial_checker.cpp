#include "halley/data_structures/rect_spatial_checker.h"
#include <climits>

using namespace Halley;

#pragma warning(disable: 4996)

RectangleSpatialChecker::RectangleSpatialChecker(int _resolution)
	: resolution(_resolution)
{
}

bool RectangleSpatialChecker::add(Rect4i rect, DataType data)
{
	Entry entry;
	entry.data = data;
	entry.rect = rect;
	entry.hashMask = 1 << (data & 0x1F); // This will "randomly" set one of the 32 bits to 1
	entries.emplace(data, entry);
	Entry& ptr = entries[data];

	return updateData(ptr, Rect4i(), rect);
}

bool RectangleSpatialChecker::remove(DataType data)
{
	auto iter = entries.find(data);
	if (iter != entries.end()) {
		auto rect = iter->second.rect;
		Entry& ptr = iter->second;

		updateData(ptr, rect, Rect4i());
		entries.erase(iter);

		return true;
	} else {
		return false;
	}
}

bool RectangleSpatialChecker::update(Rect4i newRect, DataType data)
{
	auto iter = entries.find(data);
	if (iter != entries.end()) {
		// Exists
		auto rect = iter->second.rect;
		Entry& ptr = iter->second;
		updateData(ptr, rect, newRect);
		return true;
	} else {
		// Doesn't exist, insert
		add(newRect, data);
		return false;
	}
}

bool RectangleSpatialChecker::updateData(Entry& entry, Rect4i prev, Rect4i next)
{
	if (prev == next) {
		return false; // Nothing to do here
	}
	
	int x0 = INT_MAX;
	int x1 = INT_MIN;
	int y0 = INT_MAX;
	int y1 = INT_MIN;
	Rect4i addRect;
	Rect4i delRect;
	bool hasAdd = false;
	bool hasDel = false;

	if (prev.getWidth() > 0 && prev.getHeight() > 0) {
		Vector2i p1 = pointToCell(prev.getP1());
		Vector2i p2 = pointToCell(prev.getP2());
		delRect = Rect4i(p1, p2);
		hasDel = true;
		x0 = p1.x;
		x1 = p2.x;
		y0 = p1.y;
		y1 = p2.y;
	}

	if (next.getWidth() > 0 && next.getHeight() > 0) {
		Vector2i p1 = pointToCell(next.getP1());
		Vector2i p2 = pointToCell(next.getP2());
		addRect = Rect4i(p1, p2);
		hasAdd = true;
		x0 = std::min(x0, p1.x);
		x1 = std::max(x1, p2.x);
		y0 = std::min(y0, p1.y);
		y1 = std::max(y1, p2.y);
	}
	
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			Vector2i p(x, y);
			bool removeHere = hasDel && delRect.isInside(p);
			bool addHere = hasAdd && addRect.isInside(p);
			
			auto& contents = getGridCell(x, y);

			if (addHere && removeHere) {
				// Find existing, then update it with new rect
				for (size_t i = 0; i < contents.size(); i++) {
					if (contents[i].data == entry.data) {
						contents[i].rect = next;
						break;
					}
				}
			} else if (removeHere) {
				// Remove
				//int removed = 0;
				for (size_t i = 0; i < contents.size(); i++) {
					if (contents[i].data == entry.data) {
						if (i != contents.size() - 1) {
							std::swap(contents[i], contents[contents.size() - 1]);
						}
						contents.pop_back();
						break; // Safe to break because there should only ever be one copy of each object in each cell
					}
				}
			} else if (addHere) {
				// Insert, making sure to update rect to new value, since "entry" is still outdated
				contents.push_back(entry);
				contents.back().rect = next;
			}
		}
	}

	// Update the main entry itself
	entry.rect = next;

	return true;
}

RectangleSpatialChecker::QueryResults RectangleSpatialChecker::query(Rect4i rect)
{
	QueryResults results;
	results.n = 0;

	if (sweep > 0x7FFFFFFF) {
		resetSweep();
	}
	sweep++;

	// This will work as a poor man's bloom filter
	int resultMask = 0;

	Vector2i p1 = pointToCell(rect.getP1());
	Vector2i p2 = pointToCell(rect.getP2());
	int x0 = p1.x;
	int x1 = p2.x;
	int y0 = p1.y;
	int y1 = p2.y;
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			// Go through each rect in each grid element
			auto& vec = getGridCell(x, y);
			const size_t n = vec.size();
			for (size_t i = 0; i < n; i++) {
				Entry& e = vec[i];

				// Check if they intersect
				if (e.rect.intersects(rect)) {
					// Check if it's already in the buffer
					if ((resultMask & e.hashMask) != 0) {
						for (size_t j = 0; j < results.n; j++) {
							if (resultsBuffer[j] == e.data) {
								goto endOfCheckLoop; // Oh yeah, baby. I went there.
							}
						}
					}

					// Hash it in the mask
					resultMask |= e.hashMask;

					// Insert into the buffer
					if (results.n < resultsBuffer.size()) {
						resultsBuffer[results.n] = e.data;
					} else {
						resultsBuffer.push_back(e.data);
					}
					results.n++;

					endOfCheckLoop:;
				}
			}
		}
	}

	results.results = resultsBuffer.data();
	return results;
}

void RectangleSpatialChecker::resetSweep()
{
}

Halley::Vector2i RectangleSpatialChecker::pointToCell(Vector2i point)
{
	return Vector2i(point.x >> resolution, point.y >> resolution);
}

RectangleSpatialChecker::GridCell& RectangleSpatialChecker::getGridCell(int x, int y)
{
	return grid.get(x, y);
}

