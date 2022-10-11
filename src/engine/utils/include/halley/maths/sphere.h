#include "vector3.h"

namespace Halley {
	class LineSegment;

	class Sphere {
    public:
        Sphere() = default;
        Sphere(Vector3f centre, float radius)
            : centre(centre)
            , radius(radius)
        {}

        float getRadius() const { return radius; }
        Vector3f getCentre() const { return centre; }

    	bool contains(Vector3f point) const;
        bool overlaps(const Sphere& other) const;

    	[[nodiscard]] Sphere expand(float radius) const;

        static Sphere getSpanningSphere(gsl::span<const Vector3f> points);

    private:
        Vector3f centre;
        float radius;
    };

}
