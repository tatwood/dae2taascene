#ifndef CONVERT_H_
#define CONVERT_H_

#include "objmap.h"
#include <daeu.h>
#include <taa/scene.h>

//****************************************************************************
// functions

void convert_anim(
    dae_COLLADA* collada,
    dae_animation_type* daeanim,
    taa_scene* scene,
    objmap* nodemap);

void convert_collada(
    const char* colladapath,
    dae_COLLADA* collada,
    taa_scene* scene);

void convert_geometry(
    dae_COLLADA* collada,
    dae_geometry_type* daegeom,
    taa_scene* scene,
    objmap* geommap);

int convert_image(
    const char* colladapath,
    dae_COLLADA* collada,
    dae_image_type* daeimage,
    taa_scene* scene,
    objmap* imagemap);

void convert_material(
    dae_COLLADA* collada,
    dae_material_type* daemat,
    taa_scene* scene,
    objmap* imagemap,
    objmap* matmap);

void convert_node(
    dae_COLLADA* collada,
    dae_node_type* daenode,
    int parentnode,
    int skel,
    int parentjoint,
    taa_scene* scene,
    objmap* map);

void convert_node_bindings(
    dae_COLLADA* collada,
    dae_node_type* daenode,
    taa_scene* scene,
    objmap* matmap,
    objmap* geommap,
    objmap* nodemap);

void convert_skin(
    dae_COLLADA* collada,
    dae_skin_type* daeskin,
    taa_scene* scene,
    taa_scenemesh* mesh,
    objmap* nodemap);

#endif // CONVERT_H_
