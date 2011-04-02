#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "vector.h"

class Material
{
private:
    Color ambient;
    Color diffuse;
    Color specular;
};

#endif
