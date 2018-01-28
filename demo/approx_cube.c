#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "texture.h"

#define SH_COUNT 9

typedef struct
{
    float r[SH_COUNT];
    float g[SH_COUNT];
    float b[SH_COUNT];
} ShCoeffs;

static double sh9(int i, double x, double y, double z)
{
    switch (i)
    {
        case 0:
            return 0.5 * sqrt(1.0 / M_PI);
        case 1:
            return x * 0.5 * sqrt(3.0 / M_PI);
        case 2:
            return z * 0.5 * sqrt(3.0 / M_PI);
        case 3:
            return y * 0.5 * sqrt(3.0 / M_PI);
        case 4:
            return x * z * 0.5 * sqrt(15.0 / M_PI);
        case 5:
            return y * z * 0.5 * sqrt(15.0 / M_PI);
        case 6:
            return x * y * 0.5 * sqrt(15.0 / M_PI);
        case 7:
            return (3.0 * z*z - 1.0) * 0.25 * sqrt(5.0 / M_PI);
        case 8:
            return (x*x - y*y) * 0.25 * sqrt(15.0 / M_PI);
        default:
            assert(0);
            return 0;
    }
}


static double surface_area(double x, double y)
{
    return atan2(x * y, sqrt(x * x + y * y + 1.0));
}

static ShCoeffs integrate_cubemap(Texture* cubemap)
{
    ShCoeffs coeffs;

    for (int i = 0; i < SH_COUNT; ++i)
    {
        coeffs.r[i] = 0.0;
        coeffs.g[i] = 0.0;
        coeffs.b[i] = 0.0;
    }

    double d_x = 1.0 / (double)cubemap->width;
    double d_y = d_x;
    double sum_da = 0.0;

    for (int face = 0; face < CUBE_FACE_COUNT; ++face)
    {
        size_t offset = cubemap->image_info[face].offset;

        for (int y = 0; y < cubemap->height; ++y)
        {
            for (int x = 0; x < cubemap->width; ++x)
            {
                double px = (double)x + 0.5;
                double py = (double)y + 0.5;
                double u = 2.0 * (px / (double)cubemap->width) - 1.0;
                double v = 2.0 * (px / (double)cubemap->height) - 1.0;

                double x0 = u - d_x;
                double y0 = v - d_y;
                double x1 = u + d_x;
                double y1 = v + d_y;

                double d_a = surface_area(x0, y0) - surface_area(x0, y1) - surface_area(x1, y0) + surface_area(x1, y1);

                double dir_x, dir_y, dir_z;
                switch (face)
                {
                    case CUBE_FACE_RIGHT:
                        dir_x = 1.0f;
                        dir_y = v;
                        dir_z = -u;
                        break;
                    case CUBE_FACE_LEFT:
                        dir_x = -1.0f;
                        dir_y = v;
                        dir_z = u;
                        break;
                    case CUBE_FACE_TOP:
                        dir_x = u;
                        dir_y = 1.0f;
                        dir_z = -v; 
                        break;
                    case CUBE_FACE_BOTTOM:
                        dir_x = u;
                        dir_y = -1.0f;
                        dir_z = v;
                        break;
                    case CUBE_FACE_BACK: 
                        dir_x = u;
                        dir_y = v;
                        dir_z = 1.0f;
                        break;
                    case CUBE_FACE_FRONT:
                        dir_x = -u;
                        dir_y = v;
                        dir_z = -1.0f;
                        break;
                }
                float norm = 1.0 / (dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
                dir_x *= norm;
                dir_y *= norm;
                dir_z *= norm;

                size_t pixel_start = offset + (x + y * cubemap->width) * 3;
                double red = cubemap->data[pixel_start] / 255.0;
                double green = cubemap->data[pixel_start + 1] / 255.0;
                double blue = cubemap->data[pixel_start + 2] / 255.0;

                double weight = d_a;
                sum_da += weight;

                for (int i = 0; i < SH_COUNT; ++i)
                {
                    double sh_val = sh9(i, dir_x, dir_y, dir_z);
                    coeffs.r[i] += sh_val * red * weight;
                    coeffs.g[i] += sh_val * green * weight;
                    coeffs.b[i] += sh_val * blue * weight;
                }
            }
        }
    }
    printf("%f\n", surface_area(1,1));
    printf("%f\n", sum_da);
    return coeffs;
}


int main(int argc, const char* argv[])
{
    const char* paths[] = {"data/cube1/posx.jpg",
                           "data/cube1/negx.jpg",
                           "data/cube1/posy.jpg",
                           "data/cube1/negy.jpg",
                           "data/cube1/posz.jpg",
                           "data/cube1/negz.jpg"};

    Texture cube = cubemap_load(paths);
    ShCoeffs coeffs = integrate_cubemap(&cube);

    printf("red {");
    for (int i = 0; i < SH_COUNT; ++i)
    {
        printf("%f, ", coeffs.r[i]);
    }
    printf("}\n");

    printf("green {");
    for (int i = 0; i < SH_COUNT; ++i)
    {
        printf("%f, ", coeffs.g[i]);
    }
    printf("}\n");

    printf("blue {");
    for (int i = 0; i < SH_COUNT; ++i)
    {
        printf("%f, ", coeffs.b[i]);
    }
    printf("}\n");

    return 1;
}


