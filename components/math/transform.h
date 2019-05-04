#pragma once
#include "vector.h"
#include "quaternion.h"
#include "matrix.h"

namespace tocs {
namespace math {


class transform
{
public:
	vector3 position;
	quaternion rotation;
	vector3 scale;

	transform()
		: scale(1, 1, 1)
	{}

	matrix4 VECTORCALL as_matrix() const;

	vector3 VECTORCALL transform_point(vector3 point) const;
	vector3 VECTORCALL inv_transform_point(vector3 point) const;

	vector3 VECTORCALL transform_direction(vector3 point) const;
	vector3 VECTORCALL inv_transform_direction(vector3 point) const;

	vector3 VECTORCALL forward();
	vector3 VECTORCALL left();
	vector3 VECTORCALL right();

};


}
}
