#include "convert.h"
#include <ctype.h>

//****************************************************************************
static void convert_collada_library_animations(
    dae_COLLADA* collada,
    dae_library_animations_type* daelibanim,
    taa_scene* scene,
    objmap* nodemap)
{
    dae_animation_type** daeanimitr = daelibanim->el_animation.values;
    dae_animation_type** daeanimend = daeanimitr+daelibanim->el_animation.size;
    while(daeanimitr != daeanimend)
    {
        dae_animation_type* daeanim = *daeanimitr;
        convert_anim(collada, daeanim, scene, nodemap);
        ++daeanimitr;
    }
}

//****************************************************************************
static void convert_collada_library_images(
    const char* colladapath,
    dae_COLLADA* collada,
    dae_library_images_type* daelibimage,
    taa_scene* scene,
    objmap* imagemap)
{
    dae_image_type** daeimageitr = daelibimage->el_image.values;
    dae_image_type** daeimageend = daeimageitr+daelibimage->el_image.size;
    while(daeimageitr != daeimageend)
    {
        dae_image_type* daeimage = *daeimageitr;
        convert_image(colladapath, collada, daeimage, scene, imagemap);
        ++daeimageitr;
    }
}

//****************************************************************************
static void convert_collada_library_materials(
    dae_COLLADA* collada,
    dae_library_materials_type* daelibmat,
    taa_scene* scene,
    objmap* imagemap,
    objmap* matmap)
{
    dae_material_type** daematitr = daelibmat->el_material.values;
    dae_material_type** daematend = daematitr+daelibmat->el_material.size;
    while(daematitr != daematend)
    {
        dae_material_type* daemat = *daematitr;
        convert_material(collada, daemat, scene, imagemap, matmap);
        ++daematitr;
    }
}

//****************************************************************************
static void convert_collada_library_geometries(
    dae_COLLADA* collada,
    dae_library_geometries_type* daelibgeom,
    taa_scene* scene,
    objmap* geommap)
{
    dae_geometry_type** daegeomitr = daelibgeom->el_geometry.values;
    dae_geometry_type** daegeomend = daegeomitr+daelibgeom->el_geometry.size;
    while(daegeomitr != daegeomend)
    {
        dae_geometry_type* daegeom = *daegeomitr;
        convert_geometry(collada, daegeom, scene, geommap);
        ++daegeomitr;
    }
}

//****************************************************************************
static void convert_collada_visual_scene(
    dae_COLLADA* collada,
    dae_visual_scene_type* daevscene,
    taa_scene* scene,
    objmap* matmap,
    objmap* geommap,
    objmap* nodemap)
{
    dae_node_type** daenodeitr;
    dae_node_type** daenodeend;
    // first pass, recursively convert dae nodes to scene nodes
    daenodeitr = daevscene->el_node.values;
    daenodeend = daenodeitr + daevscene->el_node.size;
    while(daenodeitr != daenodeend)
    {
        convert_node(collada,*daenodeitr,-1,-1,-1,scene,nodemap);
        ++daenodeitr;
    }
    // second pass, update all the node references from the map
    daenodeitr = daevscene->el_node.values;
    daenodeend = daenodeitr + daevscene->el_node.size;
    while(daenodeitr != daenodeend)
    {
        convert_node_bindings(
            collada,
            *daenodeitr,
            scene,
            matmap,
            geommap,
            nodemap);
        ++daenodeitr;
    }
}

//****************************************************************************
void convert_collada(
    const char* colladapath,
    dae_COLLADA* collada,
    taa_scene* scene)
{
    objmap imagemap;
    objmap matmap;
    objmap geommap;
    objmap nodemap;
    dae_library_animations_type** daelibanimitr;
    dae_library_animations_type** daelibanimend;
    dae_library_images_type** daelibimageitr;
    dae_library_images_type** daelibimageend;
    dae_library_materials_type** daelibmatitr;
    dae_library_materials_type** daelibmatend;
    dae_library_geometries_type** daelibgeomitr;
    dae_library_geometries_type** daelibgeomend;
    dae_COLLADA_scene* daescene;
    dae_visual_scene_type* daevscene;
    const char* sceneuri = NULL;
    // initialize maps
    objmap_create(&imagemap);
    objmap_create(&matmap);
    objmap_create(&geommap);
    objmap_create(&nodemap);

    // determine up axis, assuming y is default
    scene->upaxis = taa_SCENE_Y_UP;
    if(collada->el_asset != NULL)
    {
        if(collada->el_asset->el_up_axis != NULL)
        {
            const char* daeupaxis = *collada->el_asset->el_up_axis;
            if(daeupaxis != NULL)
            {
                if(toupper(daeupaxis[0]) == 'Z')
                {
                    scene->upaxis = taa_SCENE_Z_UP;
                }
            }
        }
    }
    // convert all the image libraries
    // this must occur before the materials are converted,
    // materials will reference textures
    daelibimageitr = collada->el_library_images.values;
    daelibimageend = daelibimageitr + collada->el_library_images.size;
    while(daelibimageitr != daelibimageend)
    {
        convert_collada_library_images(
            colladapath,
            collada,
            *daelibimageitr,
            scene,
            &imagemap);
        ++daelibimageitr;
    }
    // convert all the material libraries
    // this must occur before the scene is converted,
    // the scene will reference materials
    daelibmatitr = collada->el_library_materials.values;
    daelibmatend = daelibmatitr + collada->el_library_materials.size;
    while(daelibmatitr != daelibmatend)
    {
        convert_collada_library_materials(
            collada,
            *daelibmatitr,
            scene,
            &imagemap,
            &matmap);
        ++daelibmatitr;
    }
    // convert all the geometry libraries
    // this must occur before the scene is converted
    // the scene will reference geometry
    daelibgeomitr = collada->el_library_geometries.values;
    daelibgeomend = daelibgeomitr + collada->el_library_geometries.size;
    while(daelibgeomitr != daelibgeomend)
    {
        convert_collada_library_geometries(
            collada,
            *daelibgeomitr,
            scene,
            &geommap);
        ++daelibgeomitr;
    }
    // find the uri of the scene
    daescene = collada->el_scene;
    if(daescene != NULL)
    {
        dae_instance_with_extra_type* daesceneinstance;
        daesceneinstance = daescene->el_instance_visual_scene;
        if(daesceneinstance != NULL)
        {
            char** aturl = daesceneinstance->at_url;
            if(aturl != NULL)
            {
                sceneuri = *aturl;
            }
        }
    }
    // find the visual scene referenced by the uri
    daevscene = NULL;
    if(sceneuri != NULL)
    {
        dae_library_visual_scenes_type** libvsceneitr;
        dae_library_visual_scenes_type** libvsceneend;
        libvsceneitr = collada->el_library_visual_scenes.values;
        libvsceneend = libvsceneitr + collada->el_library_visual_scenes.size;
        while(libvsceneitr != libvsceneend)
        {
            dae_obj_ptr daevsceneobj;
            daevsceneobj = daeu_search_uri(*libvsceneitr, sceneuri);
            if(daevsceneobj != NULL)
            {
                if(dae_get_typeid(daevsceneobj) == dae_ID_VISUAL_SCENE_TYPE)
                {
                    daevscene = (dae_visual_scene_type*) daevsceneobj;
                }
                break;
            }
            ++libvsceneitr;
        }
    }
    // convert the visual scene
    // this must occur before the animations are converted,
    // the animations will reference scene nodes
    if(daevscene != NULL)
    {
        convert_collada_visual_scene(
            collada,
            daevscene,
            scene,
            &matmap,
            &geommap,
            &nodemap);
    }
    // convert all the animation libraries
    daelibanimitr = collada->el_library_animations.values;
    daelibanimend = daelibanimitr + collada->el_library_animations.size;
    while(daelibanimitr != daelibanimend)
    {
        convert_collada_library_animations(
            collada,
            *daelibanimitr,
            scene,
            &nodemap);
        ++daelibanimitr;
    }
    // clean up
    objmap_destroy(&imagemap);
    objmap_destroy(&matmap);
    objmap_destroy(&geommap);
    objmap_destroy(&nodemap);
}
