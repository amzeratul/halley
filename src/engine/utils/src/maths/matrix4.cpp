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

#include <cstring>
#include "halley/maths/matrix4.h"
#include "halley/maths/quaternion.h"
using namespace Halley;

Matrix4f::Matrix4f()
{
}

Matrix4f::Matrix4f(const Matrix4f& m)
	: elements(m.elements)
{
}

Matrix4f::Matrix4f(Matrix4f&& m) noexcept
	: elements(m.elements)
{
}

Matrix4f::Matrix4f(const float elems[])
{
	memcpy(elements.data(), elems, sizeof(float) * 16);
}

Matrix4f& Matrix4f::operator=(const Matrix4f& param)
{
	elements = param.elements;
	return *this;
}

Matrix4f& Matrix4f::operator*=(const Matrix4f& param)
{
	elements = ((*this) * param).elements;
	return *this;
}

Matrix4f Matrix4f::operator*(const Matrix4f& param) const
{
	Matrix4f result;
	for (size_t y = 0; y < 4; y++) {
		for (size_t x = 0; x < 4; x++) {
			float accum = 0.0f;
			for (size_t i = 0; i < 4; i++) {
				accum += getElement(i, y) * param.getElement(x, i);
			}
			result.getElement(x, y) = accum;
		}
	}
	return result;
}

Vector2f Matrix4f::operator*(const Vector2f& param) const
{
	float src[4] = { param.x, param.y, 0.0f, 1.0f };
	float result[4];
	for (size_t i = 0; i < 4; i++) {
		float accum = 0.0f;
		for (size_t j = 0; j < 4; j++) {
			accum += getElement(j, i) * src[j];
		}
		result[i] = accum;
	}
	return Vector2f(result[0] / result[3], result[1] / result[3]);
}

void Matrix4f::loadIdentity()
{
	const static float identityMatrix[] =  {1.0f, 0.0f, 0.0f, 0.0f,
											0.0f, 1.0f, 0.0f, 0.0f,
											0.0f, 0.0f, 1.0f, 0.0f,
											0.0f, 0.0f, 0.0f, 1.0f};
	memcpy(elements.data(), identityMatrix, sizeof(float) * 16);
}

void Matrix4f::rotateZ(Angle1f angle)
{
	(*this) *= makeRotationZ(angle);
}

void Matrix4f::scale(Vector2f scale)
{
	(*this) *= makeScaling(scale);
}

void Matrix4f::scale(Vector3f scale)
{
	(*this) *= makeScaling(scale);
}

void Matrix4f::translate(Vector2f translation)
{
	(*this) *= makeTranslation(translation);
}

void Matrix4f::translate(Vector3f translation)
{
	(*this) *= makeTranslation(translation);
}

void Matrix4f::transpose()
{
	auto& e = elements;
	
	std::swap(e[1], e[4]);
	std::swap(e[2], e[8]);
	std::swap(e[3], e[12]);

	std::swap(e[6], e[9]);
	std::swap(e[7], e[13]);

	std::swap(e[11], e[14]);
}

float* Matrix4f::getElements()
{
	return elements.data();
}

const float* Matrix4f::getElements() const
{
	return elements.data();
}

Matrix4f Matrix4f::makeIdentity()
{
	Matrix4f result;
	result.loadIdentity();
	return result;
}

Matrix4f Matrix4f::makeRotationX(Angle1f angle)
{
	float s, c;
	angle.sincos(s, c);
	
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(1, 1)] = c;
	result.elements[getIndex(2, 1)] = -s;
	result.elements[getIndex(1, 2)] = s;
	result.elements[getIndex(2, 2)] = c;
	return result;
}

Matrix4f Matrix4f::makeRotationY(Angle1f angle)
{
	float s, c;
	angle.sincos(s, c);
	
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(0, 0)] = c;
	result.elements[getIndex(2, 0)] = s;
	result.elements[getIndex(0, 2)] = -s;
	result.elements[getIndex(2, 2)] = c;
	return result;
}

Matrix4f Matrix4f::makeRotationZ(Angle1f angle)
{
	float s, c;
	angle.sincos(s, c);

	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(0, 0)] = c;
	result.elements[getIndex(0, 1)] = s;
	result.elements[getIndex(1, 0)] = -s;
	result.elements[getIndex(1, 1)] = c;
	return result;
}

Matrix4f Matrix4f::makeRotation(const Quaternion& q)
{
	const float a = q.a;
	const float b = q.b;
	const float c = q.c;
	const float d = q.d;
	const float mat[16] = {
		a, b, c, d,
		-b, a, d, -c,
		-c, -d, a, b,
		-d, c, -b, a
	};
	return Matrix4f(mat);
}

Matrix4f Matrix4f::makeScaling(Vector2f scale)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(0, 0)] = scale.x;
	result.elements[getIndex(1, 1)] = scale.y;
	return result;
}

Matrix4f Matrix4f::makeScaling(Vector3f scale)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(0, 0)] = scale.x;
	result.elements[getIndex(1, 1)] = scale.y;
	result.elements[getIndex(2, 2)] = scale.z;
	return result;
}

Matrix4f Matrix4f::makeTranslation(Vector2f translation)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(3, 0)] = translation.x;
	result.elements[getIndex(3, 1)] = translation.y;
	return result;
}

Matrix4f Matrix4f::makeTranslation(Vector3f translation)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[getIndex(3, 0)] = translation.x;
	result.elements[getIndex(3, 1)] = translation.y;
	result.elements[getIndex(3, 2)] = translation.z;
	return result;
}

Matrix4f Matrix4f::makeOrtho2D(float left, float right, float bottom, float top, float near, float far)
{
	const float width = right - left;
	const float height = top - bottom;
	const float depth = far - near;
	const float xc = (right + left) / width;
	const float yc = (top + bottom) / height;
	const float zc = (far + near) / depth;

	const float mat[16] = { 2.0f / width, 0.0f, 0.0f, 0.0f,
							0.0f, 2.0f / height, 0.0f, 0.0f,
							0.0f, 0.0f, -2.0f / depth, 0.0f,
							-xc, -yc, -zc, 1.0f };
	return Matrix4f(mat);
}
