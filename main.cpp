#include "main.h"


bool checkCollisions(
    Ray const &r,
    std::vector<Object *> const &all_objects,
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
        // else
        // {
        //     // std::cout << "No interesection \n";
        // }
    }
    // if (hit)
    // {
    //     // std::cout << "Finally the hit object is " << *hit_object << std::endl;
    // }
    return hit;
}

double rand01()
{
    return ((double)rand() / (RAND_MAX));
}

std::vector<Ray> sampleLightPoints(Ray const &rayToLight,
                                   Sphere const &light,
                                   int n_pts)
{
    // find an orthnormal basis of the disk
    // If rayToLight.dir ==  Vec3(0, 0, 1) big problem, too lazy to fix
    Vec3 v1 = cross_product(rayToLight.dir, Vec3(0, 0, 1));
    v1 = v1.normalize();
    Vec3 v2 = cross_product(rayToLight.dir, v1);
    v2 = v2.normalize();

    std::vector<Ray> shadowRays;
    Vec3 dir;

    std::srand(std::time(nullptr)); // use current time as seed for random generator
    for (int i = 0; i < n_pts; i++)
    {
        double angle = rand01() * 2 * M_PI;
        double dst = rand01() * light.s;
        double x = cos(angle);
        double y = sin(angle);
        Vec3 ptsOnDisk = dst * (x * v1 + y * v2);
        dir = light.pos + ptsOnDisk - rayToLight.pos;
        shadowRays.push_back(Ray(rayToLight.pos, dir, rayToLight.color, rayToLight.depth));
    }
    shadowRays.push_back(rayToLight);

    // Vec3 dir;
    // dir = light.pos + light.s * v1 - rayToLight.pos;
    // shadowRays.push_back(Ray(rayToLight.pos, dir, rayToLight.color, rayToLight.depth));
    // dir = light.pos - light.s * v1 - rayToLight.pos;
    // shadowRays.push_back(Ray(rayToLight.pos, dir, rayToLight.color, rayToLight.depth));
    // dir = light.pos + light.s * v2 - rayToLight.pos;
    // shadowRays.push_back(Ray(rayToLight.pos, dir, rayToLight.color, rayToLight.depth));
    // dir = light.pos - light.s * v2 - rayToLight.pos;
    // shadowRays.push_back(Ray(rayToLight.pos, dir, rayToLight.color, rayToLight.depth));

    return shadowRays;
}

Vec3 traceShadowRay(Ray const &rayToLight,
                    Ray const &normal_ray,
                    Vec3 const &color_surface,
                    std::vector<Object *> const &all_objects,
                    Sphere const &light)
{

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

Vec3 castShadowRay(Ray const &normal_ray,
                   Vec3 const &color_surface,
                   std::vector<Object *> const &all_objects,
                   Sphere const &light,
                   Options const &option)
{

    Ray rayToLight = Ray(normal_ray.pos, light.pos - normal_ray.pos, normal_ray.color, normal_ray.depth);

    std::vector<Ray> shadowRays = sampleLightPoints(rayToLight, light, option.n_ray_shadow);
    Vec3 res_color = Vec3(0);
    for (auto r = shadowRays.begin(); r != shadowRays.end(); r++)
    {
        res_color = res_color + traceShadowRay(*r, normal_ray, color_surface, all_objects, light);
    }
    // Could be better than averaging, like using cosine of angle as weight ?
    res_color = res_color / shadowRays.size();
    return res_color;
}

Vec3 trace(Ray const &r,
           std::vector<Object *> const &all_objects,
           Sphere const &light,
           Options const &option)
{
    // std::cout << "Tracing at depth " << r.depth << " for " << r << std::endl;
    if (r.depth >= option.MAX_DEPTH)
    {
        // return shootToLight(r, light, all_objects, n_objects);
        // return r.color;
        return sky;
    };

    double t_hit = INFINITY;
    Object *hit_object = nullptr;

    if (!checkCollisions(r, all_objects, t_hit, hit_object))
    {
        return sky * r.color; // sky-background color;
        // return sky // sky-background color;
    }

    Vec3 hitPoint = r.pos + t_hit * r.dir;
    Vec3 normal_dir = hit_object->computeNormalDir(hitPoint);
    hitPoint = hitPoint + 1e-5 * normal_dir; // To avoid self_intersection
    Vec3 color_surface = hit_object->getColorAt(hitPoint);

    Ray normal_ray = Ray(hitPoint, normal_dir, color_surface, r.depth);
    if (hit_object->isGlass)
    {

        Ray reflected_ray = computeReflectedRay(normal_ray, r, hit_object->opacity);
        Vec3 color_reflected = trace(reflected_ray, all_objects, light, option);

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

        // rougness * shadowColor + (1-rougness) * reflected_color
        Ray reflected_ray = computeReflectedRay(normal_ray, r);
        Vec3 color_reflected = trace(reflected_ray, all_objects, light, option);
        Vec3 color_shadow = castShadowRay(normal_ray, color_surface * r.color, all_objects, light, option);

        return hit_object->roughness * color_shadow + (1 - hit_object->roughness) * color_reflected;
        // return castShadowRay(normal_ray, color_surface * r.color, all_objects, light);
    }
}

Vec3 compute_pixel_color(Ray const &primaryRay,
                         std::vector<Object *> const &all_objects,
                         Sphere const &light,
                         Options const &option)
{
    Vec3 pixel_color;

    pixel_color = trace(primaryRay,
                        all_objects,
                        light,
                        option);

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

int main()
{

    Options option;
    option.HEIGHT = 2048;
    option.WIDTH = 2048;
    option.PIXEL_SIZE = 0.001;
    option.MAX_DEPTH = 4;
    option.random_alliasing = false;
    option.n_ray_per_pixel = 4;
    option.n_ray_shadow = 1;

    std::vector<std::vector<Vec3>> image(option.HEIGHT, std::vector<Vec3>(option.WIDTH));

    // Declare the objects in the scene

    Camera camera = Camera(Vec3(0, 0, 1), Vec3(-1, 0, 1.001), option.PIXEL_SIZE, option.WIDTH, option.HEIGHT);
    printf(camera);

    Sphere light = Sphere(Vec3(-2, -4, 3), WHITE, 0.1);

    std::vector<Object *> all_objects;
    Object *ground = new CheckBoard();
    Object *sphere1 = new Sphere(Vec3(2, 0, 1), BLUE, 1);
    Sphere *sphere2 = new Sphere(Vec3(1, 1, 0.5), GREEN, 0.5);
    Sphere *sphere3 = new Sphere(Vec3(1, -1, 0.5), RED, 0.5);

    ground->roughness = 0.8;

    all_objects.push_back(ground);
    all_objects.push_back(sphere1);
    all_objects.push_back(sphere2);
    all_objects.push_back(sphere3);

    // fill the pixel values
    ProgressBar pbar = ProgressBar(70, option.WIDTH * option.HEIGHT);

    for (int i = 0; i < option.HEIGHT; i++)
    {
        for (int j = 0; j < option.WIDTH; j++)
        {
            std::vector<Ray *> rays;

            // anti-alliasing :
            if (option.random_alliasing)
            {
                rays = camera.shoot_rays(i, j);
            }
            else
            {
                rays = camera.shoot_random_rays(i, j, option.n_ray_per_pixel);
            }

            int alliasing_factor = rays.size();
            Vec3 color_pixel = Vec3(0);
            for (int k = 0; k < alliasing_factor; k++)
            {
                Ray *r = rays.at(k);
                color_pixel = color_pixel + compute_pixel_color(*r, all_objects, light, option);
                delete r;
            }
            image[i][j] = color_pixel / alliasing_factor;
            rays.clear();

            pbar.update();
        }
    }

    for (auto o = all_objects.begin(); o != all_objects.end(); o++)
    {
        delete *o;
    }
    all_objects.clear();

    // print_image(image, option.WIDTH, option.HEIGHT);
    write_image(image, option.WIDTH, option.HEIGHT);

    return 0;
};