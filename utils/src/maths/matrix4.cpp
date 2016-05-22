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
using namespace Halley;

Matrix4f::Matrix4f()
{
}

Matrix4f::Matrix4f(const Matrix4f& m)
	: elements(m.elements)
{
}

Matrix4f::Matrix4f(Matrix4f&& m)
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

Halley::Vector2f Halley::Matrix4f::operator*(const Vector2f& param) const
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

void Matrix4f::scale2D(float x, float y)
{
	(*this) *= makeScaling2D(x, y);
}

void Matrix4f::translate2D(float x, float y)
{
	(*this) *= makeTranslation2D(x, y);
}

void Halley::Matrix4f::transpose()
{
	(*this) *= makeTranspose();
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

Matrix4f Matrix4f::makeRotationZ(Angle1f angle)
{
	float s, c;
	angle.sincos(s, c);

	Matrix4f result;
	result.loadIdentity();
	result.elements[0] = c;
	result.elements[1] = s;
	result.elements[4] = -s;
	result.elements[5] = c;
	return result;
}

Matrix4f Matrix4f::makeScaling2D(float scaleX, float scaleY)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[0] = scaleX;
	result.elements[5] = scaleY;
	return result;
}

Matrix4f Matrix4f::makeTranslation2D(float x, float y)
{
	Matrix4f result;
	result.loadIdentity();
	result.elements[12] = x;
	result.elements[13] = y;
	return result;
}

Halley::Matrix4f Halley::Matrix4f::makeOrtho2D(float left, float right, float bottom, float top, float _near, float _far)
{
	// Replacement for glOrtho(), as that doesn't exist in OpenGL ES
	// See http://www.khronos.org/opengles/documentation/opengles1_0/html/glOrtho.html
	// Remember, this matrix looks transposed
	const float mat[16] = { 2.0f/(right-left), 0.0f, 0.0f, 0.0f,
							0.0f, 2.0f/(top-bottom), 0.0f, 0.0f,
							0.0f, 0.0f, -2.0f / (_far-_near), 0.0f,
							-(right+left)/(right-left), -(top+bottom)/(top-bottom), -(_far+_near)/(_far-_near), 1.0f };
	return Matrix4f(mat);
}

Halley::Matrix4f Halley::Matrix4f::makeTranspose()
{
	const float mat[16] = { 0.0f, 1.0f, 0.0f, 0.0f,
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f };
	return Matrix4f(mat);
}
