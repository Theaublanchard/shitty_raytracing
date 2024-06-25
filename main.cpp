#include "main.h"

int MAX_DEPTH = 4;

Vec3 WHITE = Vec3(1);
Vec3 BLACK = Vec3(0);
Vec3 RED = Vec3(1, 0, 0);
Vec3 GREEN = Vec3(0, 1, 0);
Vec3 BLUE = Vec3(0, 0, 1);
Vec3 GREY = 0.5 * WHITE + 0.5 * BLACK;
Vec3 sky = GREY;

bool checkCollisions(
    Ray r,
    std::vector<Object *> all_objects,
    double &t_hit,
    Object *&hit_object)
{
    // std::cout << "Checking collisions for " << r << std::endl;

    double curr_thit = INFINITY;
    bool hit = false;
    int n_obj = all_objects.size();
    for (int i = 0; i < n_obj; i++)
    {
        Object *curr_obj = all_objects.at(i);

        // std::cout << "Checking collision for object " << i << " : " << *curr_obj << std::endl;
        // std::cout << "Currently we have t_hit = " << t_hit << std::endl;

        if (curr_obj->intersect(r, curr_thit))
        {
            // std::cout << "Found intersection with " << *curr_obj << " at " << curr_thit << std::endl;
            // printf("So now we have currthit = %f and t_hit = %f \n", curr_thit, t_hit);
            if (curr_thit < t_hit)
            {
                t_hit = curr_thit;
                hit = true;
                hit_object = curr_obj;
            }
        }
        else
        {
            // std::cout << "No interesection \n";
        }
    }
    if (hit)
    {
        // std::cout << "Finally the hit object is " << *hit_object << std::endl;
    }
    return hit;
}

// double castShadowRay(Ray const &normal_ray, Vec3 const &color_surface, std::vector<Object *> all_objects, Sphere const &light)
Vec3 castShadowRay(Ray const &normal_ray, Vec3 const &color_surface, std::vector<Object *> all_objects, Sphere const &light)
{
    // std::cout << "\nChecking for light visibility\n";

    Ray rayToLight = Ray(normal_ray.pos, light.pos - normal_ray.pos, normal_ray.color, normal_ray.depth);

    /*
    TODO:
    to have better shadow, shoot multiple rays ~towards the light.
    An option would be to sample points on the disk centered at the position
    of the light and orthogonal to rayToLight.dir.
    Then average the values.
    Could also be possible to have a weighted average of the point
    according to the cosine of the angle with the true ray direction.
    */

    double t_hit = INFINITY;
    Object *hit_obj = nullptr;
    bool blocked = checkCollisions(rayToLight, all_objects, t_hit, hit_obj);

    if (blocked)
    {
        if (hit_obj->isGlass)
        {
            float facing_ratio = rayToLight.dir.normalize().dot(normal_ray.dir);
            return facing_ratio * light.color * color_surface * (1 - hit_obj->opacity);

            // shoot another reflected ray i guess;
            // Vec3 hitPoint = rayToLight.pos + t_hit * rayToLight.dir;
            // Vec3 normal_dir = hit_obj->computeNormalDir(hitPoint);
            // hitPoint = hitPoint + 1e-5 * normal_dir; // To avoid self_intersection
            // Vec3 color_glass = hit_obj->getColorAt(hitPoint);
            // Vec3 color_surface = color_glass * rayToLight.color;
            // Ray normal_ray = Ray(hitPoint, normal_dir, color_surface, rayToLight.depth);

            // Ray reflected_ray = computeReflectedRay(normal_ray, rayToLight, hit_obj->opacity);
            // Vec3 color_reflected = trace(reflected_ray, all_objects, light);
            // return color_reflected;
        }
        else
        {
            // something in the way, return black.
            // std::cout << "The light is not visible.\n";
            return Vec3(0);
        }
    }
    else
    {
        // the light is visible
        float facing_ratio = rayToLight.dir.normalize().dot(normal_ray.dir);
        // std::cout << "The light is visible.\n";
        // return facing_ratio;
        return facing_ratio * light.color * color_surface;
    }
}

Vec3 trace(Ray r, std::vector<Object *> all_objects, Sphere light)
{
    // std::cout << "Tracing at depth " << r.depth << " for " << r << std::endl;
    if (r.depth >= MAX_DEPTH)
    {
        // return shootToLight(r, light, all_objects, n_objects);
        return r.color;
    };

    double t_hit = INFINITY;
    Object *hit_object = nullptr;

    if (!checkCollisions(r, all_objects, t_hit, hit_object))
    {
        return sky * r.color; // sky-background color;
    }

    Vec3 hitPoint = r.pos + t_hit * r.dir;
    Vec3 normal_dir = hit_object->computeNormalDir(hitPoint);
    hitPoint = hitPoint + 1e-5 * normal_dir; // To avoid self_intersection
    Vec3 color_surface = hit_object->getColorAt(hitPoint);

    Ray normal_ray = Ray(hitPoint, normal_dir, color_surface, r.depth);
    if (hit_object->isGlass)
    {

        Ray reflected_ray = computeReflectedRay(normal_ray, r, hit_object->opacity);
        Vec3 color_reflected = trace(reflected_ray, all_objects, light);

        // if (hit_object->opacity < 1)
        // {
        //     Ray transmitted_ray = computeTransmittedRay(normal_ray, r, hit_object);
        //     Vec3 color_transmitted = trace(transmitted_ray, all_objects, light);
        //     return (color_reflected + color_transmitted) * 0.5;
        // }
        return r.color * color_reflected;
    }
    else
    {
        return castShadowRay(normal_ray, color_surface * r.color, all_objects, light);
    }
}

Vec3 compute_pixel_color(
    Ray primaryRay,
    std::vector<Object *> all_objects,
    Sphere light)
{
    Vec3 pixel_color;

    pixel_color = trace(primaryRay,
                        all_objects,
                        light);

    return pixel_color;
}

void print_image(const std::vector<std::vector<Vec3>> &image, int WIDTH, int HEIGHT)
{
    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            std::cout << image[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

double clamp(double low, double high, double val)
{
    return std::max(std::min(val, high), low);
}

void write_image(const std::vector<std::vector<Vec3>> &image,
                 int WIDTH, int HEIGHT)
{
    const double max_value = 255;
    // Save result to a PPM image (keep these flags if you compile under Windows)
    std::ofstream ofs("./out.ppm", std::ios::out | std::ios::binary);

    // Check if the file stream is open
    if (!ofs.is_open())
    {
        std::cerr << "Error: Could not open the file!" << std::endl;
    }
    ofs << "P6\n"
        << WIDTH << " " << HEIGHT << "\n"
        << int(max_value) << "\n";
    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            char r = (char)(255 * clamp(0, 1, image[i][j].x));
            char g = (char)(255 * clamp(0, 1, image[i][j].y));
            char b = (char)(255 * clamp(0, 1, image[i][j].z));
            ofs << r << g << b;
        }
    }

    ofs.close();
};

int WIDTH = 2048;
int HEIGHT = 2048;
double PIXEL_SIZE = 0.001;

int main()
{
    std::vector<std::vector<Vec3>> image(HEIGHT, std::vector<Vec3>(WIDTH));

    // Declare the objects in the scene

    Camera camera = Camera(Vec3(0, 0, 1), Vec3(-1, 0, 1.001), PIXEL_SIZE, WIDTH, HEIGHT);
    printf(camera);

    Sphere light = Sphere(Vec3(-2, -4, 3), WHITE, 3);

    std::vector<Object *> all_objects;
    all_objects.push_back(new CheckBoard());
    all_objects.push_back(new Sphere(
        Vec3(2, 0, 1), BLUE, 1));
    Sphere *glassSphere = new Sphere(Vec3(1, 1, 0.5), GREEN, 0.5);
    glassSphere->isGlass = false;
    glassSphere->opacity = 0.3;
    all_objects.push_back(glassSphere);
    Sphere *glassSphere2 = new Sphere(Vec3(1, -1, 0.5), RED, 0.5);
    glassSphere2->isGlass = false;
    glassSphere2->opacity = 0.6;
    all_objects.push_back(glassSphere2);

    // fill the pixel values
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            // anti-alliasing :
            std::vector<Ray *> rays = camera.shoot_rays(i, j);
            int alliasing_factor = rays.size();
            Vec3 color_pixel = Vec3(0);
            for (int k = 0; k < alliasing_factor; k++)
            {
                Ray *r = rays.at(k);
                color_pixel = color_pixel + compute_pixel_color(*r, all_objects, light);
                delete r;
            }
            image[i][j] = color_pixel / alliasing_factor;
            rays.clear();
        }
    }

    for (auto o = all_objects.begin(); o != all_objects.end(); o++)
    {
        delete *o;
    }
    all_objects.clear();

    // print_image(image, WIDTH, HEIGHT);
    write_image(image, WIDTH, HEIGHT);

    return 0;
};