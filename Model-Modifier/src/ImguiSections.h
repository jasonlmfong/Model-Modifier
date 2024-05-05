#pragma once

#include "scene/object/ObjectSelect.h"
#include <string>
#include <utility>
#include <vector>


std::vector<std::pair<unsigned int, const char *>> geometricObjectNames
{
    { CRUMPLED,    "Crumbled"    },
    { CUBE,        "Cube"        },
    { DOUBLETORUS, "Double torus"},
    { FANDISK,     "Fandisk"     },
    { ICOSA,       "Icosahedron" },
    { OCTA,        "Octahedron"  },
    { OLOID,       "Oloid"       },
    { SPHERE,      "Sphere"      },
    { STAR,        "Star"        },
    { T,           "T-Shape"     },
    { TORUS,       "Torus"       },
    { TUBES,       "Tubes"       }
};

std::vector<std::pair<unsigned int, const char*>> modelObjectNames
{
    { ANKYLOSAURUS, "Ankylosaurus"},
    { ARMADILLO,    "Armadillo"   },
    { BOB,          "Bob (Tri)"   },
    { BOB2,         "Bob (Quad)"  },
    { BUNNY,        "Bunny"       },
    { COW,          "Cow (Tri)"   },
    { COW2,         "Cow (Mixed)" },
    { COWHEAD,      "Cow head"    },
    { FACE,         "Face"        },
    { GARGOYLE,     "Gargoyle"    },
    { KITTEN,       "Kitten"      },
    { SHUTTLE,      "Shuttle"     },
    { SUZANNE,      "Suzanne"     },
    { TEAPOT,       "Teapot"      },
    { TEDDY,        "Teddy"       }
};
