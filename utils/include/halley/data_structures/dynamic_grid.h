#pragma once

#include <vector>

namespace Halley {
	template <typename T>
	class DynamicGrid {
	public:
		DynamicGrid() {
			makeGrid(-8, 8, -8, 8);
		}

		T& get(int x, int y) {
			if (x < minX || x >= maxX || y < minY || y >= maxY) {
				resizeToFit(x, y);
			}
			return getElement(x, y);
		}

	private:
		std::vector<T> grid;
		int minX = 0;
		int maxX = 0;
		int minY = 0;
		int maxY = 0;

		void makeGrid(int _minX, int _maxX, int _minY, int _maxY) {
			minX = _minX;
			maxX = _maxX;
			minY = _minY;
			maxY = _maxY;
			int w = maxX - minX;
			int h = maxY - minY;
			grid.clear();
			grid.resize(w * h);
		}

		T& getElement(int x, int y) {
			int w = maxX - minX;
			int idx = (x - minX) + (y - minY) * w;
			return grid[idx];
		}

		void resizeToFit(int x, int y) {
			// Compute new bounds
			int x0 = minX;
			int x1 = maxX;
			int y0 = minY;
			int y1 = maxY;
			while (x < x0) {
				x0 *= 2;
			}
			while (x >= x1) {
				x1 *= 2;
			}
			while (y < y0) {
				y0 *= 2;
			}
			while (y >= y1) {
				y1 *= 2;
			}

			// Store data
			std::vector<T> oldGrid = std::move(grid);

			// Compute copy parameters
			int oldX = minX;
			int oldY = minY;
			int oldW = maxX - minX;
			int oldH = maxY - minY;

			// Allocate
			makeGrid(x0, x1, y0, y1);

			// Copy data
			for (int cy = 0; cy < oldH; cy++) {
				for (int cx = 0; cx < oldW; cx++) {
					getElement(oldX + cx, oldY + cy) = std::move(oldGrid[cx + cy * oldW]);
				}
			}
		}
	};
}
