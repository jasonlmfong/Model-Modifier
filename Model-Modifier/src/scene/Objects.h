#pragma once

#include <unordered_map>

#include "Object.h"

enum objectEnum
{
    ARMADILLO,
    BUNNY,
    COWHEAD,
    CUBE,
    DOUBLETORUS,
    FACE,
    GARGOYLE,
    ICOSA,
    KITTEN,
    OCTA,
    SHUTTLE,
    SPHERE,
    STAR,
    SUZANNE,
    T,
    TEAPOT,
    TEDDY,
    TORUS,
    TUBES
};

class Objects
{
public:
    Objects();
	~Objects();
    Object findObj(int obj);

public:
    std::unordered_map<int, const char*> m_Filepaths
    {
        { ARMADILLO, "res/objects/armadillo.obj" },
        { BUNNY, "res/objects/bunny.obj" },
        { COWHEAD, "res/objects/cowhead.obj" },
        { CUBE, "res/objects/cube.obj" },
        { DOUBLETORUS,"res/objects/double-torus.obj" },
        { FACE, "res/objects/face.obj" },
        { GARGOYLE, "res/objects/gargoyle.obj" },
        { ICOSA, "res/objects/ico.obj" },
        { KITTEN, "res/objects/kitten.obj" },
        { OCTA, "res/objects/octa.obj" },
        { SHUTTLE, "res/objects/shuttle.obj" },
        { SPHERE, "res/objects/sphere.obj" },
        { STAR, "res/objects/star.obj" },
        { SUZANNE, "res/objects/suzanne.obj" },
        { T, "res/objects/T.obj" },
        { TEAPOT, "res/objects/teapot.obj" },
        { TEDDY, "res/objects/teddy.obj" },
        { TORUS, "res/objects/torus.obj" },
        { TUBES, "res/objects/tubes.obj" }
    };
    std::unordered_map<int, Object> m_Objects;
};