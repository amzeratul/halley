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
#include "vector3.h"
#include "vector4.h"

namespace Halley {
	class Quaternion;

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
		Vector3f operator*(const Vector3f& param) const;
		Vector4f operator*(const Vector4f& param) const;

		static Matrix4f makeIdentity();
		static Matrix4f makeBase(Vector3f x, Vector3f y, Vector3f z);

		static Matrix4f makeRotationX(Angle1f angle);
		static Matrix4f makeRotationY(Angle1f angle);
		static Matrix4f makeRotationZ(Angle1f angle);
		static Matrix4f makeRotation(const Quaternion& rotation);
		static Matrix4f makeScaling(Vector2f scale);
		static Matrix4f makeScaling(Vector3f scale);
		static Matrix4f makeTranslation(Vector2f translation);
		static Matrix4f makeTranslation(Vector3f translation);

		static Matrix4f makeOrtho2D(float left, float right, float bottom, float top, float near, float far);
		static Matrix4f makePerspective(float near, float far, float aspectRatio, const Angle1f& fov);

		void loadIdentity();
		void rotateZ(Angle1f angle);
		void rotate(const Quaternion& quaternion);
		void scale(Vector2f scale);
		void scale(Vector3f scale);
		void translate(Vector2f translation);
		void translate(Vector3f translation);
		void transpose();

		Quaternion toRotationQuaternion() const;

		float* getElements();
		const float* getElements() const;
		
		float getElement(size_t column, size_t row) const
		{
			return columns[column][row];
		}

		inline float& getElement(size_t column, size_t row)
		{
			return columns[column][row];
		}

		Vector4f getRow(size_t row) const;
		Vector4f getColumn(size_t column) const;

	private:
		std::array<Vector4f, 4> columns;
	};
}
