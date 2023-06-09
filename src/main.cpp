#include <cmath>
#include <future>

#include <PAMImage/PPM.hpp>
#include <SLib/SLog.hpp>

#include "Camera.hpp"
#include "Ray.hpp"
#include "Sphere.hpp"
#include "Scene.h"
#include "Random.hpp"
#include "Color.h"
#include "Lambert.h"
#include "Metal.hpp"

Color rayColor(const Ray& ray, const Scene& scene, int depth){
    if (depth == 0){
        return {0, 0, 0};
    }

    Hit hit = scene.Hit(ray, 0.0001, std::numeric_limits<double>::infinity());
    Color attenuation;

    if (hit) {
        auto scattered = hit.material->Scatter(ray, hit, attenuation);
        return attenuation * rayColor(scattered, scene, depth - 1);
    } else {
        Vec3 unit_direction = (ray.direction).unit();
        double t = 0.5*(unit_direction.j + 1.0);
        return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
    }
}

int main() {
    S_LOG_LEVEL_INFO;

    std::size_t width = 256;
    std::size_t height = 256;
    auto aspect = double(width) / double(height);

    const int samples = 512;
    const int depth = 3;
    const int threads = 8;

    auto material1 = std::make_shared<Lambert>(Color(0.8, 0.8, 0.0));
    auto material2 = std::make_shared<Lambert>(Color(0.0, 0.8, 0.8));
    auto material3 = std::make_shared<Lambert>(Color(0.5, 0.5, 0.5));

    auto material4 = std::make_shared<Metal>(Color(0.5, 0.5, 0.5), 0);
    auto material5 = std::make_shared<Metal>(Color(0.5, 0.5, 0.5), 0.5);

    Camera camera(Vec3(3,5,3), Vec3(0,0,0), Vec3(0,-1,0), 60, aspect);
    Scene scene;
    scene.Add(std::make_shared<Sphere>(Vec3(-2,1,0), 1, material1));
    scene.Add(std::make_shared<Sphere>(Vec3(0,1,0), 1, material2));
    scene.Add(std::make_shared<Sphere>(Vec3(2,1,0), 1, material3));

    scene.Add(std::make_shared<Sphere>(Vec3(-2,1,2), 1, material4));
    scene.Add(std::make_shared<Sphere>(Vec3(0,1,2), 1, material5));
    scene.Add(std::make_shared<Sphere>(Vec3(2,1,2), 1, material1));

    scene.Add(std::make_shared<Sphere>(Vec3(-2,1,-2), 1, material2));
    scene.Add(std::make_shared<Sphere>(Vec3(0,1,-2), 1, material4));
    scene.Add(std::make_shared<Sphere>(Vec3(2,1,-2), 1, material5));

    scene.Add(std::make_shared<Sphere>(Vec3(0,-1000,0), 1000, material3));

    pam::PPM ppm(width, height, pam::PPM::Max16);

    std::vector<std::future<void>> tasks;

    auto waitTasks = [&tasks](){
        while (!tasks.empty()){
            tasks.back().wait();
            tasks.pop_back();
        }
    };

    S_INFO("Start rendering...");
    for (std::size_t i = 0; i < width; ++i) {
        if ((i + 1) % threads == 0){
            waitTasks();
        }

        tasks.emplace_back(std::async(std::launch::async, [i, height, width, &camera, &scene, &ppm](){
            Random random;

            for (std::size_t j = 0; j < height; ++j) {
                Color color;
                for (int k = 0; k < samples; ++k) {
                    auto u = (i + random.Get()) / (width - 1);
                    auto v = (j + random.Get()) / (height - 1);

                    Ray ray = camera.getRay(u, v);
                    color += rayColor(ray, scene, depth);
                }

                auto scale = 1.0 / samples;
                color.i = sqrt(scale * color.i);
                color.j = sqrt(scale * color.j);
                color.k = sqrt(scale * color.k);

                auto r = static_cast<std::uint16_t>(color.i * pam::PPM::Max16);
                auto g = static_cast<std::uint16_t>(color.j * pam::PPM::Max16);
                auto b = static_cast<std::uint16_t>(color.k * pam::PPM::Max16);

                ppm(j, i) = {r, g, b};
            }
        }));

        S_INFO("Done: " + std::to_string(double(i) * 100 / (width - 1)) + "%");
    }

    waitTasks();

    S_INFO("End rendering...");

    ppm.Write("out.ppm");

    return 0;
}
