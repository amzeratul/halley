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
#include "halley/maths/simd.h"
#include "halley/maths/quaternion.h"
#include "halley/support/logger.h"
using namespace Halley;

Matrix4f::Matrix4f()
	: columns({ Vector4f(Vector4f::Uninitialized()), Vector4f(Vector4f::Uninitialized()), Vector4f(Vector4f::Uninitialized()), Vector4f(Vector4f::Uninitialized()) })
{
}

Matrix4f::Matrix4f(const Matrix4f& m)
	: columns(m.columns)
{
}

Matrix4f::Matrix4f(Matrix4f&& m) noexcept
	: columns(std::move(m.columns))
{
}

Matrix4f::Matrix4f(const float elems[])
{
	memcpy(getElements(), elems, sizeof(float) * 16);
}

Matrix4f& Matrix4f::operator=(const Matrix4f& param) = default;

Matrix4f& Matrix4f::operator*=(const Matrix4f& param)
{
	*this = (*this) * param;
	return *this;
}

Matrix4f Matrix4f::operator*(const Matrix4f& param) const
{
	Matrix4f result;

	auto r0 = SIMDVec4::loadAligned(getColumn(0).data());
	auto r1 = SIMDVec4::loadAligned(getColumn(1).data());
	auto r2 = SIMDVec4::loadAligned(getColumn(2).data());
	auto r3 = SIMDVec4::loadAligned(getColumn(3).data());
	SIMDVec4::transpose(r0, r1, r2, r3);

	for (size_t x = 0; x < 4; x++) {
		auto c = SIMDVec4::loadAligned(param.getColumn(x).data());

		// Computing output column "x"
		auto d0 = r0 * c;
		auto d1 = r1 * c;
		d0 = SIMDVec4::horizontalAdd(d0, d1);
		auto d2 = r2 * c;
		auto d3 = r3 * c;
		d2 = SIMDVec4::horizontalAdd(d2, d3);
		d0 = SIMDVec4::horizontalAdd(d0, d2);

		d0.storeAligned(result.columns[x].data());
	}
	return result;
}

Vector2f Matrix4f::operator*(const Vector2f& param) const
{
	return ((*this) * Vector4f(param, 0.0f, 1.0f)).toVector2();
}

Vector3f Matrix4f::operator*(const Vector3f& param) const
{
	return ((*this) * Vector4f(param, 1.0f)).toVector3();
}

Vector4f Matrix4f::operator*(const Vector4f& v) const
{
	auto r0 = SIMDVec4::loadAligned(getRow(0).data());
	auto r1 = SIMDVec4::loadAligned(getRow(1).data());
	auto r2 = SIMDVec4::loadAligned(getRow(2).data());
	auto r3 = SIMDVec4::loadAligned(getRow(3).data());

	auto c = SIMDVec4::loadAligned(v.data());
	
	auto d0 = r0 * c;
	auto d1 = r1 * c;
	d0 = SIMDVec4::horizontalAdd(d0, d1);
	auto d2 = r2 * c;
	auto d3 = r3 * c;
	d2 = SIMDVec4::horizontalAdd(d2, d3);
	d0 = SIMDVec4::horizontalAdd(d0, d2);

	auto result = Vector4f(Vector4f::Uninitialized());
	d0.storeAligned(result.data());
	return result;
}

void Matrix4f::loadIdentity()
{
	const static float identityMatrix[] =  {1.0f, 0.0f, 0.0f, 0.0f,
											0.0f, 1.0f, 0.0f, 0.0f,
											0.0f, 0.0f, 1.0f, 0.0f,
											0.0f, 0.0f, 0.0f, 1.0f};
	memcpy(getElements(), identityMatrix, sizeof(float) * 16);
}

void Matrix4f::rotateZ(Angle1f angle)
{
	(*this) *= makeRotationZ(angle);
}

void Matrix4f::rotate(const Quaternion& quaternion)
{
	*this *= makeRotation(quaternion);
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
	auto e = getElements();
	
	std::swap(e[1], e[4]);
	std::swap(e[2], e[8]);
	std::swap(e[3], e[12]);

	std::swap(e[6], e[9]);
	std::swap(e[7], e[13]);

	std::swap(e[11], e[14]);
}

Quaternion Matrix4f::toRotationQuaternion() const
{
	// From https://stackoverflow.com/questions/52413464/look-at-quaternion-using-up-vector/52551983#52551983
	// Untested

	const auto& e = [&] (int x, int y) { return getElement(x, y); };
	
	const float t0 = e(0, 0);
	const float t1 = e(1, 1);
	const float t2 = e(2, 2);
	const float trace = t0 + t1 + t2;
	
	if (trace > 0) {
		const float s = 0.5f / std::sqrt(trace + 1.0f);
		return Quaternion(
			0.25f / s,
			(e(2, 1) - e(1, 2)) * s,
			(e(0, 2) - e(2, 0)) * s,
			(e(1, 0) - e(0, 1)) * s);
	} else {
		if (t0 > t1 && t0 > t2) {
			const float s = 2.0f * std::sqrt(1.0f + t0 - t1 - t2);
			return Quaternion(
				(e(2, 1) - e(1, 2)) / s,
				0.25f * s,
				(e(0, 1) + e(1, 0)) / s,
				(e(0, 2) + e(2, 0)) / s
			);
		} else if (t1 > t2) {
			const float s = 2.0f * std::sqrt(1.0f + t1 - t0 - t2);
			return Quaternion(
				(e(0, 2) - e(2, 0)) / s,
				(e(0, 1) + e(1, 0)) / s,
				0.25f * s,
				(e(1, 2) + e(2, 1)) / s
			);
		} else {
			const float s = 2.0f * std::sqrt(1.0f + t2 - t0 - t1);
			return Quaternion(
				(e(1, 0) - e(0, 1)) / s,
				(e(0, 2) + e(2, 0)) / s,
				(e(1, 2) + e(2, 1)) / s,
				0.25f * s
			);
		}
	}
}

float* Matrix4f::getElements()
{
	return &(columns[0][0]);
}

const float* Matrix4f::getElements() const
{
	return &(columns[0][0]);
}

Vector4f Matrix4f::getRow(size_t row) const
{
	return Vector4f(getElement(0, row), getElement(1, row), getElement(2, row), getElement(3, row));
}

Vector4f Matrix4f::getColumn(size_t column) const
{
	return columns[column];
}

Matrix4f Matrix4f::makeIdentity()
{
	Matrix4f result;
	result.loadIdentity();
	return result;
}

Matrix4f Matrix4f::makeBase(Vector3f x, Vector3f y, Vector3f z)
{
	// Untested

	/*
	const float mat[16] = {
		x.x, x.y, x.z, 0,
		y.x, y.y, y.z, 0,
		z.x, z.y, z.z, 0,
		0,   0,   0,   1
	};
	*/
	const float mat[16] = {
		x.x, y.x, z.x, 0,
		x.y, y.y, z.y, 0,
		x.z, y.z, z.z, 0,
		0,   0,   0,   1
	};
	return Matrix4f(mat);
}

Matrix4f Matrix4f::makeRotationX(Angle1f angle)
{
	const float s = angle.sin();
	const float c = angle.cos();
	
	Matrix4f result;
	result.loadIdentity();
	result.columns[1][1] = c;
	result.columns[2][1] = -s;
	result.columns[1][2] = s;
	result.columns[2][2] = c;
	return result;
}

Matrix4f Matrix4f::makeRotationY(Angle1f angle)
{
	const float s = angle.sin();
	const float c = angle.cos();
	
	Matrix4f result;
	result.loadIdentity();
	result.columns[0][0] = c;
	result.columns[2][0] = s;
	result.columns[0][2] = -s;
	result.columns[2][2] = c;
	return result;
}

Matrix4f Matrix4f::makeRotationZ(Angle1f angle)
{
	const float s = angle.sin();
	const float c = angle.cos();

	Matrix4f result;
	result.loadIdentity();
	result.columns[0][0] = c;
	result.columns[0][1] = s;
	result.columns[1][0] = -s;
	result.columns[1][1] = c;
	return result;
}

Matrix4f Matrix4f::makeRotation(const Quaternion& q)
{
	// From https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
	const float r = q.w;
	const float i = q.x;
	const float j = q.y;
	const float k = q.z;
	const float mat[16] = {
		1 - 2*(j*j + k*k), 2*(i*j + k*r), 2*(i*k - j*r), 0,
		2*(i*j - k*r), 1 - 2*(i*i + k*k), 2*(j*k + i*r), 0,
		2*(i*k + j*r), 2*(j*k - i*r), 1 - 2*(i*i + j*j), 0,
		0, 0, 0, 1
	};

	return Matrix4f(mat);
}

Matrix4f Matrix4f::makeScaling(Vector2f scale)
{
	Matrix4f result;
	result.loadIdentity();
	result.columns[0][0] = scale.x;
	result.columns[1][1] = scale.y;
	return result;
}

Matrix4f Matrix4f::makeScaling(Vector3f scale)
{
	Matrix4f result;
	result.loadIdentity();
	result.columns[0][0] = scale.x;
	result.columns[1][1] = scale.y;
	result.columns[2][2] = scale.z;
	return result;
}

Matrix4f Matrix4f::makeTranslation(Vector2f translation)
{
	Matrix4f result;
	result.loadIdentity();
	result.columns[3][0] = translation.x;
	result.columns[3][1] = translation.y;
	return result;
}

Matrix4f Matrix4f::makeTranslation(Vector3f translation)
{
	Matrix4f result;
	result.loadIdentity();
	result.columns[3][0] = translation.x;
	result.columns[3][1] = translation.y;
	result.columns[3][2] = translation.z;
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

Matrix4f Matrix4f::makePerspective(float near, float far, float aspectRatio, const Angle1f& fov)
{
	const float tan = (fov * 0.5f).tan();
	const float mat[16] = { 1.0f / (aspectRatio * tan), 0, 0, 0,
							0, 1.0f / tan, 0, 0,
							0, 0, (-near - far)/(near - far), 1,
							0, 0, (2 * far * near)/(near - far), 0 };
	return Matrix4f(mat);
}
