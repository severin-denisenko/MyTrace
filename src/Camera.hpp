//
// Created by Severin on 27.03.2023.
//

#ifndef MYTRACE_CAMERA_HPP
#define MYTRACE_CAMERA_HPP

#include <cstdlib>

#include <Algebra3D/Vec3.hpp>

#include "Ray.hpp"

using algebra::Vec3;

class Camera {
public:
    Camera(Vec3 from, Vec3 at, Vec3 up, double fov, double aspect);

    Ray getRay(double u, double v) const;

private:
    Vec3 origin = {.0, .0, .0};
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 lowerLeftCorner;
};

#endif //MYTRACE_CAMERA_HPP
