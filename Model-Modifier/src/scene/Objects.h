#pragma once

#include <unordered_map>

#include "Object.h"

enum objectEnum
{
    BUNNY,
    COWHEAD,
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
    TEAPOT,
    TEDDY,
    TORUS
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
        { BUNNY, "res/objects/bunny.obj" },
        { COWHEAD, "res/objects/cowhead.obj" },
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
        { TEAPOT, "res/objects/teapot.obj" },
        { TEDDY, "res/objects/teddy.obj" },
        { TORUS, "res/objects/torus.obj" }
    };
    std::unordered_map<int, Object> m_Objects;
};