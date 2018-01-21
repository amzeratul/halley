/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <array>
#include "angle.h"
#include "vector2.h"

namespace Halley {
	class Matrix4f {
	public:
		Matrix4f();
		Matrix4f(const Matrix4f& m);
		Matrix4f(Matrix4f&& m) noexcept;
		Matrix4f(const float elements[]);

		Matrix4f& operator=(const Matrix4f& param);
		Matrix4f& operator*=(const Matrix4f& param);
		Matrix4f operator*(const Matrix4f& param) const;
		Vector2f operator*(const Vector2f& param) const;

		static Matrix4f makeIdentity();
		static Matrix4f makeRotationZ(Angle1f angle);
		static Matrix4f makeScaling2D(float scaleX, float scaleY);
		static Matrix4f makeTranslation2D(float x, float y);
		static Matrix4f makeOrtho2D(float left, float right, float bottom, float top, float near, float far);
		static Matrix4f makeTranspose();

		void loadIdentity();
		void rotateZ(Angle1f angle);
		void scale2D(float x, float y);
		void translate2D(float x, float y);
		void transpose();

		float* getElements();
		const float* getElements() const;

		inline float getElement(size_t column, size_t row) const
		{
			return elements[4 * column + row];
		}

		inline float& getElement(size_t column, size_t row)
		{
			return elements[4 * column + row];
		}

	private:
		std::array<float, 16> elements;
	};
}