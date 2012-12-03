#include "convert.h"
#include <taa/log.h>
#include <taa/scalar.h>
#include <taa/vec2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//****************************************************************************
static void convert_anim_channel(
    dae_COLLADA* collada,
    dae_channel_type* daechannel,
    taa_scene* scene,
    taa_sceneanim* anim,
    objmap* nodemap)
{
    int err = 0;
    dae_sampler_type* daesampler;
    dae_float_array_type* daeinputarray;
    dae_float_array_type* daeoutputarray;
    dae_float_array_type* daeintanarray;
    dae_float_array_type* daeouttanarray;
    dae_name_array_type* daeinterparray;
    dae_obj_typeid daetargettype;
    uint8_t compmask[16];
    int nodeid;
    int numcomp;

    // attempt to find sampler
    daesampler = NULL;
    if(daechannel->at_source != NULL)
    {
        const char* uri = *daechannel->at_source;
        if(uri != NULL)
        {
            dae_obj_ptr daesourceobj;
            daesourceobj = daeu_search_uri(collada, uri);
            if(daesourceobj != NULL)
            {
                if(dae_get_typeid(daesourceobj) == dae_ID_SAMPLER_TYPE)
                {
                    daesampler = (dae_sampler_type*) daesourceobj;
                }
            }
        }
    }

    daeinputarray = NULL;
    daeoutputarray = NULL;
    daeintanarray = NULL;
    daeouttanarray = NULL;
    daeinterparray = NULL;
    if(daesampler != NULL)
    {
        dae_input_local_type** daeinputitr;
        dae_input_local_type** daeinputend;
        daeinputitr = daesampler->el_input.values;
        daeinputend = daeinputitr + daesampler->el_input.size;
        while(daeinputitr != daeinputend)
        {
            dae_input_local_type* daeinput = *daeinputitr;
            dae_source_type* daesource = NULL; 
            const char* semantic = NULL;
            if(daeinput->at_source != NULL)
            {
                const char* uri = *daeinput->at_source;
                if(uri != NULL)
                {
                    dae_obj_ptr daesourceobj;
                    daesourceobj = daeu_search_uri(collada, uri);
                    if(daesourceobj != NULL)
                    {
                        if(dae_get_typeid(daesourceobj) == dae_ID_SOURCE_TYPE)
                        {
                            daesource = (dae_source_type*) daesourceobj;
                        }
                    }
                }
            }
            if(daeinput->at_semantic != NULL)
            {
                semantic = *daeinput->at_semantic;
            }
            if(semantic != NULL && daesource != NULL)
            {
                if(!strcmp(semantic, "INPUT"))
                {
                    daeinputarray = daesource->el_float_array;
                }
                if(!strcmp(semantic, "OUTPUT"))
                {
                    daeoutputarray = daesource->el_float_array;
                }
                if(!strcmp(semantic, "IN_TANGENT"))
                {
                    daeintanarray = daesource->el_float_array;
                }
                if(!strcmp(semantic, "OUT_TANGENT"))
                {
                    daeouttanarray = daesource->el_float_array;
                }
                if(!strcmp(semantic, "INTERPOLATION"))
                {
                    daeinterparray = daesource->el_Name_array;
                }
            }
            ++daeinputitr;
        }
    }

    // attempt to find target
    daetargettype = dae_ID_INVALID;
    memset(compmask, 0, sizeof(compmask));
    numcomp = 0;
    nodeid = -1;
    if(daechannel->at_target != NULL)
    {
        const char* ref = *daechannel->at_target;
        if(ref != NULL)
        {
            dae_obj_ptr daetargetobj;
            int daedataindex;
            int numtargets;
            numtargets = daeu_search_sid(
                collada,
                ref,
                &daetargetobj,
                &daedataindex);
            if(numtargets > 0)
            {
                nodeid = objmap_find(nodemap, daetargetobj);
                daetargettype = dae_get_typeid(daetargetobj);
                if(nodeid > 0)
                {
                    taa_scenenode* node = scene->nodes + nodeid;
                    switch(node->type)
                    {
                    case taa_SCENENODE_TRANSFORM_MATRIX:
                        if(numtargets==1)
                        {
                            for(numcomp = 0; numcomp < 16; ++numcomp)
                            {
                                compmask[numcomp] = 1;
                            }
                        }
                        break;
                    case taa_SCENENODE_TRANSFORM_ROTATE:
                        if(numtargets==2 && daedataindex==3)
                        {
                            numcomp = 1;
                            compmask[3] = 1;
                        }
                        break;
                    case taa_SCENENODE_TRANSFORM_SCALE:
                        if(numtargets == 2)
                        {
                            if(daedataindex < 3)
                            {
                                numcomp = 1;
                                compmask[daedataindex] = 1;
                            }
                        }
                        else if(numtargets == 1)
                        {
                            numcomp = 3;
                            compmask[0] = compmask[1] = compmask[2] = 1;
                        }
                        break;
                    case taa_SCENENODE_TRANSFORM_TRANSLATE:
                        if(numtargets == 2)
                        {
                            if(daedataindex < 3)
                            {
                                numcomp = 1;
                                compmask[daedataindex] = 1;
                            }
                        }
                        else if(numtargets == 1)
                        {
                            numcomp = 3;
                            compmask[0] = compmask[1] = compmask[2] = 1;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    if(numcomp == 0)
    {
        taa_LOG_ERROR("invalid anim target node");
    }

    // validate arrays
    if(daeinputarray != NULL)
    {
        if(daeoutputarray != NULL)
        {
            if(daeinputarray->data.size != daeoutputarray->data.size/numcomp)
            {
                taa_LOG_ERROR("invalid output array");
                err = -1;
            }
        }
        else
        {
            taa_LOG_ERROR("missing output array");
            err = -1;
        }
    }
    else
    {
        taa_LOG_ERROR("missing input array");
        err = -1;
    }
    if(err == 0)
    {
        if(daeinterparray != NULL)
        {
            if(daeinputarray->data.size != daeinterparray->data.size)
            {
                taa_LOG_ERROR("invalid interpolation array");
                err = -1;
            }
        }
        else
        {
            taa_LOG_ERROR("missing interpolation array");
            err = -1;
        }
    }
    if(err == 0)
    {
        if(daeintanarray != NULL)
        {
            if(daeinputarray->data.size != daeintanarray->data.size*2)
            {
                if(daeinputarray->data.size != daeintanarray->data.size)
                {
                    taa_LOG_ERROR("invalid input tangent array");
                    err = -1;
                }
            }
        }
        else if(daeouttanarray != NULL)
        {
            taa_LOG_ERROR("missing input tangent array");
            err = -1;
        }
    }
    if(err == 0)
    {
        if(daeouttanarray != NULL)
        {
            if(daeinputarray->data.size != daeouttanarray->data.size*2)
            {
                if(daeinputarray->data.size != daeouttanarray->data.size)
                {
                    taa_LOG_ERROR("invalid output tangent array");
                    err = -1;
                }
            }
        }
        else if(daeintanarray != NULL)
        {
            taa_LOG_ERROR("missing output tangent array");
            err = -1;
        }
    }

    if(err == 0)
    {
        taa_sceneanim_channel* channel;
        int channelid;
        unsigned i;
        unsigned int numkeys;
        channelid = taa_sceneanim_add_channel(anim,numcomp,compmask,nodeid);
        channel = anim->channels + channelid;
        numkeys = daeinputarray->data.size;
        for(i = 0; i < numkeys; ++i)
        {
            const char* daeinterp = daeinterparray->data.values[i];
            float in = daeinputarray->data.values[i];
            float out[16];
            taa_sceneanim_interpolation interp;
            taa_vec2 cpin;
            taa_vec2 cpout;
            int comp;
            for(comp = 0; comp < numcomp; ++comp)
            {
                out[comp] = daeoutputarray->data.values[i*numcomp + comp];
            }
            if(daetargettype == dae_ID_MATRIX_TYPE)
            {
                // if the target is matrix, need to transpose into column
                // vectors
                taa_mat44 m;
                taa_mat44 mt;
                assert(numcomp == 16);
                memcpy(&m, out, sizeof(m));
                taa_mat44_transpose(&m, &mt);
                memcpy(out, &mt, sizeof(out));
            }
            else if(daetargettype == dae_ID_ROTATE_TYPE)
            {
                // if the target is a rotation angle, 
                // need to convert to radians
                assert(numcomp == 1);
                assert(compmask[3] == 1);
                *out = taa_radians(*out);
            }
            // determine interpolation type
            if(!strcmp(daeinterp,"BEZIER"))
            {
                if(daeintanarray != NULL && daeouttanarray != NULL)
                {
                    interp = taa_SCENEANIM_INTERPOLATE_BEZIER;
                }
                else
                {
                    taa_LOG_ERROR("bezier interpolation without tangents");
                    interp = taa_SCENEANIM_INTERPOLATE_LINEAR;
                }
            }
            else if(!strcmp(daeinterp, "STEP"))
            {
                interp = taa_SCENEANIM_INTERPOLATE_STEP;
            }
            else
            {
                interp = taa_SCENEANIM_INTERPOLATE_LINEAR;
            }
            // calculate control points
            if(interp == taa_SCENEANIM_INTERPOLATE_BEZIER)
            {
                if(daeintanarray->data.size == numkeys*2)
                {
                    cpin.x = daeintanarray->data.values[i*2 + 0];
                    cpin.y = daeintanarray->data.values[i*2 + 1];
                }
                else
                {
                    // degenerate tangent
                    cpin.x = in;
                    cpin.y = daeintanarray->data.values[i];
                    if(i > 0)
                    {
                        const float previn = daeinputarray->data.values[i-1];
                        cpin.x = (previn*2.0f+in)/3.0f;
                    }
                }
                if(daeouttanarray->data.size == numkeys*2)
                {
                    cpout.x = daeouttanarray->data.values[i*2 + 0];
                    cpout.y = daeouttanarray->data.values[i*2 + 1];
                }
                else
                {
                    // degenerate tangent
                    cpout.x = in;
                    cpout.y = daeouttanarray->data.values[i];
                    if(i+1 < numkeys)
                    {
                        const float nextin = daeinputarray->data.values[i+1]; 
                        cpout.x = (in+nextin*2.0f)/3.0f;
                    }
                }
            }
            else
            {
                memset(&cpin, 0, sizeof(cpin));
                memset(&cpout, 0, sizeof(cpout));
            }
            taa_sceneanim_add_frame(anim,channel,interp,in,out,&cpin,&cpout);
        }
    }
    else
    {
        taa_LOG_ERROR("invalid animation");
    }
}

//****************************************************************************
static void convert_anim_recursive(
    dae_COLLADA* collada,
    dae_animation_type* daeanim,
    taa_scene* scene,
    taa_sceneanim* anim,
    objmap* nodemap)
{
    dae_animation_type** daechilditr;
    dae_animation_type** daechildend;
    dae_channel_type** daechannelitr;
    dae_channel_type** daechannelend;
    // since animations may contain other animations, parse recursively
    daechilditr = daeanim->el_animation.values;
    daechildend = daechilditr+daeanim->el_animation.size;
    while(daechilditr != daechildend)
    {
        convert_anim_recursive(
            collada,
            *daechilditr,
            scene,
            anim,
            nodemap);
        ++daechilditr;
    }
    // convert the channels
    daechannelitr = daeanim->el_channel.values;
    daechannelend = daechannelitr + daeanim->el_channel.size;
    while(daechannelitr != daechannelend)
    {
        convert_anim_channel(
            collada,
            *daechannelitr,
            scene,
            anim,
            nodemap);
        ++daechannelitr;
    }
}

//****************************************************************************
void convert_anim(
    dae_COLLADA* collada,
    dae_animation_type* daeanim,
    taa_scene* scene,
    objmap* nodemap)
{
    int animid = 0;
    if(scene->numanimations == 0)
    {
        animid = taa_scene_add_animation(scene, "");
    }
    convert_anim_recursive(
        collada,
        daeanim,
        scene,
        scene->animations + animid,
        nodemap);
}
