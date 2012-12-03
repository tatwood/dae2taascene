#include "convert.h"
#include <taa/log.h>
#include <taa/path.h>
#include <taa/uri.h>
#include <stdio.h>

//****************************************************************************
static dae_image_type* convert_material_newparam(
    dae_COLLADA* collada,
    dae_effect_type* daeeffect,
    dae_fx_common_newparam_type* daenewparam)
{
    dae_image_type* daeimage = NULL;
    if(daenewparam->el_sampler2D != NULL)
    {
        // backwards compatibility with 1.4.1
        dae_fx_sampler2D_type* daesamp2d = daenewparam->el_sampler2D;
        const char* sourceuri = daesamp2d->el_source[0];
        if(sourceuri != NULL)
        {
            dae_obj_ptr srcobj;
            if(daeu_search_sid(daeeffect, sourceuri, &srcobj, NULL) == 1)
            {
                if(dae_get_typeid(srcobj) == dae_ID_FX_COMMON_NEWPARAM_TYPE)
                {
                    daenewparam = (dae_fx_common_newparam_type*) srcobj;
                }
            }
        }
    }
    if(daenewparam->el_surface != NULL)
    {
        // backwards compatibility with 1.4.1
        dae_fx_surface_common_type* daesurf = daenewparam->el_surface;
        if(daesurf != NULL)
        {
            if(daesurf->el_init_from.size > 0)
            {
                dae_image_type_init_from* daeinit;
                daeinit = daesurf->el_init_from.values[0];
                if(daeinit->el_ref != NULL && daeinit->el_ref[0] != NULL)
                {
                    const char* inituri = daeinit->el_ref[0];
                    dae_obj_ptr initobj;
                    if(daeu_search_sid(collada,inituri,&initobj,NULL) == 1)
                    {
                        if(dae_get_typeid(initobj) == dae_ID_IMAGE_TYPE)
                        {
                             daeimage = (dae_image_type*) initobj;
                        }
                    }
                }
            }
        }
    }
    return daeimage;
}

//****************************************************************************
static int convert_material_texture(
    dae_COLLADA* collada,
    dae_effect_type* daeeffect,
    dae_fx_common_color_or_texture_type* daeparam,
    objmap* imagemap)
{
    int32_t textureid = -1;
    dae_image_type* daeimage;
    const char* ref;
    // try to find the sid reference for the image
    ref = NULL;
    if(daeparam->el_texture != NULL)
    {
        dae_fx_common_color_or_texture_type_texture* daetex;
        daetex = daeparam->el_texture;
        if(daetex->at_texture != NULL)
        {
            ref = *daetex->at_texture;
        }
    }
    // search for the collada image by the sid reference
    daeimage = NULL;
    if(ref != NULL)
    {
        dae_obj_ptr daerefobj;
        if(daeu_search_sid(collada, ref, &daerefobj, NULL) == 1)
        {
            if(dae_get_typeid(daerefobj) == dae_ID_FX_COMMON_NEWPARAM_TYPE)
            {
                dae_fx_common_newparam_type* daenewparam;
                daenewparam = (dae_fx_common_newparam_type*) daerefobj;
                daeimage = convert_material_newparam(
                    collada,
                    daeeffect,
                    daenewparam);
            }
            if(dae_get_typeid(daerefobj) == dae_ID_IMAGE_TYPE)
            {
                daeimage = (dae_image_type*) daerefobj;
            }
        }
    }
    // if image was found, find the texture id associated with it
    if(daeimage != NULL)
    {
        textureid = objmap_find(imagemap, daeimage);
    }
    else if(daeparam->el_texture != NULL)
    {
        taa_LOG_ERROR("could not find image for texture");
    }
    return textureid;
}

//****************************************************************************
void convert_material(
    dae_COLLADA* collada,
    dae_material_type* daemat,
    taa_scene* scene,
    objmap* imagemap,
    objmap* matmap)
{
    taa_scenematerial* mat;
    const char* name;
    dae_effect_type* daeeffect;
    dae_profile_common_type_technique* daetech;
    int matid;
    // create the material
    name = NULL;
    if(daemat->at_name != NULL)
    {
        name = *daemat->at_name;
    }
    if(name == NULL && daemat->at_id != NULL)
    {
        name = *daemat->at_id;
    }
    if(name == NULL)
    {
        name = "";
    }
    matid = taa_scene_add_material(scene, name);
    mat = scene->materials + matid;
    // find the associated collada effect
    daeeffect = NULL;
    if(daemat->el_instance_effect != NULL)
    {
        dae_instance_effect_type* daeinsteffect = daemat->el_instance_effect;
        if(daeinsteffect->at_url != NULL)
        {
            const char* uri = *daeinsteffect->at_url;
            if(uri != NULL)
            {
                dae_obj_ptr daeeffectobj = daeu_search_uri(collada, uri);
                if(daeeffectobj != NULL)
                {
                    if(dae_get_typeid(daeeffectobj) == dae_ID_EFFECT_TYPE)
                    {
                        daeeffect = (dae_effect_type*) daeeffectobj;
                    }
                }
            }
        }
    }
    // find the common profile technique with the effect
    daetech = NULL;
    if(daeeffect != NULL)
    {
        if(daeeffect->el_profile_COMMON != NULL)
        {
            daetech = daeeffect->el_profile_COMMON->el_technique;
        }
    }
    // try to determine what kind of material it is
    if(daetech->el_blinn != NULL)
    {
        dae_fx_common_color_or_texture_type* daediffuse;
        daediffuse = daetech->el_blinn->el_diffuse;
        if(daediffuse != NULL)
        {
            mat->diffusetexture = convert_material_texture(
                collada,
                daeeffect,
                daediffuse,
                imagemap);
        }
    }
    else if(daetech->el_constant != NULL)
    {
        // TODO: add support
        assert(0);
    }
    else if(daetech->el_lambert != NULL)
    {
        dae_fx_common_color_or_texture_type* daediffuse;
        daediffuse = daetech->el_lambert->el_diffuse;
        if(daediffuse != NULL)
        {
            mat->diffusetexture = convert_material_texture(
                collada,
                daeeffect,
                daediffuse,
                imagemap);
        }
    }
    else if(daetech->el_phong != NULL)
    {
        dae_fx_common_color_or_texture_type* daediffuse;
        daediffuse = daetech->el_phong->el_diffuse;
        if(daediffuse != NULL)
        {
            mat->diffusetexture = convert_material_texture(
                collada,
                daeeffect,
                daediffuse,
                imagemap);
        }
    }
    else
    {
        taa_LOG_ERROR("unsupported shader");
    }
    // insert the material into the conversion map
    objmap_insert(matmap, daemat, matid);
}
