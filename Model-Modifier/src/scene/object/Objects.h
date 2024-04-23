#pragma once

#include <unordered_map>

#include "Object.h"

enum objectEnum
{
    ANKYLOSAURUS,
    ARMADILLO,
    BOB,
    BOB2,
    BUNNY,
    COW,
    COW2,
    COWHEAD,
    CRUMPLED,
    CUBE,
    DOUBLETORUS,
    FACE,
    FANDISK,
    GARGOYLE,
    ICOSA,
    KITTEN,
    OCTA,
    OLOID,
    SHUTTLE,
    SPHERE,
    STAR,
    SUZANNE,
    T,
    TEAPOT,
    TEDDY,
    TORUS,
    TUBES,
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
        { ANKYLOSAURUS, "res/objects/ankylosaurus.obj" },
        { ARMADILLO, "res/objects/armadillo.obj" },
        { BOB, "res/objects/bob.obj" },

        { BOB2, "res/objects/bob2.obj" },
        { BUNNY, "res/objects/bunny.obj" },
        { COW, "res/objects/cow.obj" },

        { COW2, "res/objects/cow2.obj" },
        { COWHEAD, "res/objects/cowhead.obj" },
        { CRUMPLED, "res/objects/CrumpledDevelopable.obj" },
        { CUBE, "res/objects/cube.obj" },
        { DOUBLETORUS,"res/objects/double-torus.obj" },
        { FACE, "res/objects/face.obj" },
        { FANDISK, "res/objects/fandisk.obj" },
        { GARGOYLE, "res/objects/gargoyle.obj" },
        { ICOSA, "res/objects/ico.obj" },
        { KITTEN, "res/objects/kitten.obj" },
        { OCTA, "res/objects/octa.obj" },
        { OLOID, "res/objects/oloid64_tri.obj" },
        { SHUTTLE, "res/objects/shuttle.obj" },
        { SPHERE, "res/objects/sphere.obj" },
        { STAR, "res/objects/star.obj" },
        { SUZANNE, "res/objects/suzanne.obj" },
        { T, "res/objects/T.obj" },
        { TEAPOT, "res/objects/teapot.obj" },
        { TEDDY, "res/objects/teddy.obj" },
        { TORUS, "res/objects/torus.obj" },
        { TUBES, "res/objects/tubes.obj" },
    };
    std::unordered_map<int, Object> m_Objects;
};