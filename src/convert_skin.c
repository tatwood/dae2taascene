#include "convert.h"
#include <taa/log.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void convert_skin(
    dae_COLLADA* collada,
    dae_skin_type* daeskin,
    taa_scene* scene,
    taa_scenemesh* mesh,
    objmap* nodemap)
{
    // v' = sum(i,n) { jw_i * (JM_i * (invBM_i * (BSM*v))) }
    // i       = joint index
    // n       = number of joint weights for vertex v
    // jw_i    = weight of joint i on vertex v
    // JM_i    = matrix of joint i
    // invBM_i = inverse bind matrix of joint i
    // BSM     = bind shape matrix
    dae_name_array_type* daejointarray; // bind matrix joint names
    dae_float_array_type* daeinvbindarray; // inverse bind matrices
    dae_list_of_uints_type* daevcount; // index counts per vertex
    dae_list_of_ints_type* daev; // joint and weight indices per vertex
    dae_name_array_type* daevjarray; // vertex joint names
    dae_float_array_type* daevwarray; // vertex weights
    taa_mat44 bindshapemat;
    int daevstride; // number of v indices per vertex
    int daevjoffset; // number of joint index within v vertex
    int daevwoffset; // number of weight index within v vertex
    int vsposid;
    int indexmapping;
    int numvertices;

    // attempt to find the joints and inverse bind matrices input arrays
    daejointarray = NULL;
    daeinvbindarray = NULL;
    if(daeskin->el_joints != NULL)
    {
        dae_input_local_type** daeinputitr;
        dae_input_local_type** daeinputend;
        daeinputitr = daeskin->el_joints->el_input.values;
        daeinputend = daeinputitr + daeskin->el_joints->el_input.size;
        while(daeinputitr != daeinputend)
        {
            dae_input_local_type* daeinput = *daeinputitr;
            dae_source_type* daesource = NULL;
            // find the source for the current input object
            if(daeinput->at_source != NULL)
            {
                const char* source = *daeinput->at_source;
                if(source != NULL)
                {
                    dae_obj_ptr daesourceobj;
                    daesourceobj = daeu_search_uri(collada, source);
                    if(daesourceobj != NULL)
                    {
                        if(dae_get_typeid(daesourceobj) == dae_ID_SOURCE_TYPE)
                        {
                            daesource = (dae_source_type*) daesourceobj;
                        }
                    }
                }
            }
            // assign the source array to a variable according to its semantic
            if(daesource != NULL && daeinput->at_semantic != NULL)
            {
                const char* semantic = *daeinput->at_semantic;
                if(semantic != NULL)
                {
                    if(!strcmp(semantic, "JOINT"))
                    {
                        daejointarray = daesource->el_Name_array;
                    }
                    else if(!strcmp(semantic, "INV_BIND_MATRIX"))
                    {
                        daeinvbindarray = daesource->el_float_array;
                        if(daeinvbindarray != NULL)
                        {
                            if((daeinvbindarray->data.size & 15) != 0)
                            {
                                // only use the inverse bind matrix array if
                                // its size is divisible into 4x4 matrices
                                daeinvbindarray = NULL;
                            }
                        }
                    }
                }
            }
            ++daeinputitr;
        }
    }

    if(daeskin->el_bind_shape_matrix != NULL)
    {
        // collada matrices are specified using row vectors. to be compatible
        // with the math lib, they must be transposed to column vector
        taa_mat44 daebindmat;
        memcpy(&daebindmat,daeskin->el_bind_shape_matrix,sizeof(daebindmat));
        taa_mat44_transpose(&daebindmat, &bindshapemat);
    }
    else
    {
        taa_mat44_identity(&bindshapemat);
    }

    // map joint names and inverse bind matrices to the animation skeleton
    if(daejointarray != NULL && daeinvbindarray != NULL)
    {
        char** daenameitr = daejointarray->data.values;
        char** daenameend = daenameitr + daejointarray->data.size;
        float* daeinvbinditr = daeinvbindarray->data.values;
        float* daeinvbindend = daeinvbinditr + daeinvbindarray->data.size;
        int skelid = -1;
        while(daenameitr != daenameend)
        {
            const char* daename = (*daenameitr!=NULL) ? *daenameitr : "";
            dae_obj_ptr daenodeobj = NULL;
            int nodeid = -1;
            int jointid = -1;
            taa_mat44 invbindmat;
            // find the collada node associated with the joint name
            if(daeu_search_sid(collada, daename, &daenodeobj, NULL) > 0)
            {
                // map the collada node to a scene node
                nodeid = objmap_find(nodemap, daenodeobj);
            }
            // find the anim joint for the node
            if(nodeid >= 0)
            {
                // find the skeleton associated with the scene node
                int nodeskelid = taa_scene_find_skel_from_node(scene, nodeid);
                if(skelid < 0)
                {
                    skelid = nodeskelid;
                    mesh->skeleton = skelid;
                }
                if(nodeskelid >= 0 && nodeskelid == skelid)
                {
                    // if the skeleton matches the other joints, find the
                    // joint within the skeleton associated with the node
                    jointid = scene->nodes[nodeid].value.jointid;
                }
                else
                {
                    taa_LOG_ERROR("invalid skeleton for joint: %s", daename);
                }
            }
            else
            {
                taa_LOG_ERROR("invalid skin joint: %s", daename);
            }
            // convert the inverse bind matrix
            if((daeinvbinditr + 16) <= daeinvbindend)
            {
                // collada matrices are specified using row vectors. to be 
                // compatible with the math lib, they must be transposed to
                // column vectors
                taa_mat44 daeinvbindmat;
                memcpy(&daeinvbindmat, daeinvbinditr, sizeof(daeinvbindmat));
                taa_mat44_transpose(&daeinvbindmat, &invbindmat);
            }
            else
            {
                taa_mat44_identity(&invbindmat);
                taa_LOG_ERROR("invalid inverse bind matrix: %s", daename);
            }
            // add the skin joint to the mesh
            if(jointid >= 0)
            {
                // bake the bind shape matrix into the inverse bind matrix
                taa_mat44 bakedmat;
                taa_mat44_multiply(&invbindmat, &bindshapemat, &bakedmat);
                taa_scenemesh_add_skinjoint(mesh, jointid, &bakedmat);
            }
            else
            {
                taa_LOG_ERROR("invalid joint: %s ", daename);
            }
            ++daenameitr;
            daeinvbinditr += 16;
        }
    }
    else if(daejointarray == NULL)
    {
        taa_LOG_ERROR("invalid skin joint array");
    }
    else
    {
        taa_LOG_ERROR("invalid inverse bind matrix array");
    }

    // attempt to find inputs for vertex joint indices and vertex weights
    daevcount = NULL;
    daev = NULL;
    daevjarray = NULL;
    daevwarray = NULL;
    daevstride = 0;
    daevjoffset = 0;
    daevwoffset = 0;
    if(daeskin->el_vertex_weights != NULL)
    {
        dae_skin_type_vertex_weights* daevertweights;
        dae_input_local_offset_type** daeinputitr;
        dae_input_local_offset_type** daeinputend;
        daevertweights = daeskin->el_vertex_weights;
        // save vcount and v
        daevcount = daevertweights->el_vcount;
        daev = daevertweights->el_v;
        daevstride = daevertweights->el_input.size;
        // find the arrays
        daeinputitr =  daevertweights->el_input.values;
        daeinputend = daeinputitr + daevertweights->el_input.size;
        while(daeinputitr != daeinputend)
        {
            dae_input_local_offset_type* daeinput = *daeinputitr;
            dae_source_type* daesource = NULL;
            // find the source for the current input object
            if(daeinput->at_source != NULL)
            {
                const char* source = *daeinput->at_source;
                if(source != NULL)
                {
                    dae_obj_ptr daesourceobj;
                    daesourceobj = daeu_search_uri(collada, source);
                    if(daesourceobj != NULL)
                    {
                        if(dae_get_typeid(daesourceobj) == dae_ID_SOURCE_TYPE)
                        {
                            daesource = (dae_source_type*) daesourceobj;
                        }
                    }
                }
            }
            if(daesource != NULL && daeinput->at_semantic != NULL)
            {
                const char* semantic = *daeinput->at_semantic;
                if(semantic != NULL)
                {
                    if(!strcmp(semantic, "JOINT"))
                    {
                        daevjarray = daesource->el_Name_array;
                        if(daeinput->at_offset != NULL)
                        {
                            daevjoffset = *daeinput->at_offset;
                        }
                    }
                    else if(!strcmp(semantic, "WEIGHT"))
                    {
                        daevwarray = daesource->el_float_array;
                        if(daeinput->at_offset != NULL)
                        {
                            daevwoffset = *daeinput->at_offset;
                        }
                    }
                }
            }
            ++daeinputitr;
        }
    }

    // find the position stream of the mesh
    // the number of joint and weight verts must match the number of positions
    indexmapping = 0;
    numvertices = 0;
    vsposid = taa_scenemesh_find_stream(mesh,taa_SCENEMESH_USAGE_POSITION,0);
    if(vsposid >= 0)
    {
        const taa_scenemesh_stream* vspos = mesh->vertexstreams + vsposid;
        indexmapping = vspos->indexmapping;
        numvertices = vspos->numvertices;
    }
    else
    {
        taa_LOG_ERROR("mesh position stream not found");
    }

    // create the joint index and weight vertex streams
    if(daevcount!=NULL && daev!=NULL && daevjarray!=NULL && daevwarray!=NULL)
    {
        unsigned int* daevcountitr = daevcount->data.values;
        unsigned int* daevcountend = daevcountitr + daevcount->data.size;
        int* daevitr = daev->data.values;
        int* daevend = daevitr + daev->data.size;
        int vsjointid;
        int vsweightid;
        int* joints;
        float* weights;
        // create the vertex streams (needs same vert count as position)
        vsjointid = taa_scenemesh_add_stream(
                mesh,
                "JOINT",
                taa_SCENEMESH_USAGE_BLENDINDEX,
                0,
                taa_SCENEMESH_VALUE_INT32,
                4,
                sizeof(int32_t)*4,
                indexmapping,
                numvertices,
                NULL);
        vsweightid = taa_scenemesh_add_stream(
                mesh,
                "WEIGHT",
                taa_SCENEMESH_USAGE_BLENDWEIGHT,
                0,
                taa_SCENEMESH_VALUE_FLOAT32,
                4,
                sizeof(float)*4,
                indexmapping,
                numvertices,
                NULL);
        joints = (int*) mesh->vertexstreams[vsjointid].buffer;
        weights = (float*) mesh->vertexstreams[vsweightid].buffer;
        // need to manually convert each vertex to ensure that there are
        // 4 weights per vertex.
        while(daevcountitr != daevcountend)
        {
            // loop through each vertex
            // <vcount> and <v> are shared by both the index and weight array
            // <vcount> stores the number of v indices for each vertex <v>
            // <v> specifies the index into the index or weight array
            // there is one vcount record per vertex
            // total v items for a vertex = vcount * number indices per vert
            int* daevertend;
            int sortedjoints[5] = { 0, 0, 0, 0, 0 };
            float sortedweights[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            taa_vec4 vwscaled;
            int daevcount = *daevcountitr;
            // loop through all the v indices for this vertex
            daevertend = daevitr + daevcount*daevstride;
            while(daevitr < daevertend && daevitr < daevend)
            {
                // loop through each weight assigned to the vertex
                int daevjindex;
                int daevwindex;
                int sortindex;
                float daeweight;
                // look up the indices of the joint name and vertex weight
                daevjindex = daevitr[daevjoffset];
                daevwindex = daevitr[daevwoffset];
                if(((size_t) daevjindex) >= daevjarray->data.size)
                {
                    taa_LOG_ERROR("invalid vertex joint index");
                    daevjindex = 0;
                }
                if(((size_t) daevwindex) >= daevwarray->data.size)
                {
                    taa_LOG_ERROR("invalid vertex weight index");
                    daevwindex = 0;
                }
                daeweight = daevwarray->data.values[daevwindex];
                if(daejointarray != daevjarray)
                {
                    // if the bind matrix joint array and vertex joint array
                    // do not match, find the joint in the bind matrix array
                    char** jointitr = daejointarray->data.values;
                    char** jointend = jointitr + daejointarray->data.size;
                    const char* vjname = daevjarray->data.values[daevjindex];
                    daevjindex = -1;
                    while(jointitr != jointend)
                    {
                        if(vjname != NULL && *jointitr != NULL)
                        {
                            if(!strcmp(vjname, *jointitr))
                            {
                                char** begin = daejointarray->data.values;
                                daevjindex = (int)(ptrdiff_t)(jointitr-begin);
                                break;
                            }
                        }
                        ++jointitr;
                    }
                }
                if(daevjindex < 0)
                {
                    taa_LOG_ERROR("invalid vertex joint");
                }
                // put the weight and joint index in the vertex's sorted list
                sortindex = 4;
                while(sortindex > 0 && daeweight > sortedweights[sortindex-1])
                {
                    sortedweights[sortindex] = sortedweights[sortindex - 1];
                    sortedjoints [sortindex] = sortedjoints [sortindex - 1];
                    sortedweights[sortindex - 1] = daeweight;
                    sortedjoints [sortindex - 1] = daevjindex;
                    --sortindex;
                }
                daevitr += daevstride;
            }
            // scale the weights so that when added together the total is 1.0
            // note that this is not the same as normalizing
            vwscaled.x = sortedweights[0];
            vwscaled.y = sortedweights[1];
            vwscaled.z = sortedweights[2];
            vwscaled.w = sortedweights[3];
            if(vwscaled.x > 0.0f)
            {
                float s = vwscaled.x+vwscaled.y+vwscaled.z+vwscaled.w;
                taa_vec4_scale(&vwscaled, 1.0f/s, &vwscaled);
            }
            // copy the sorted vertex data into the stream buffer
            memcpy(joints, sortedjoints, 4*sizeof(*joints));
            memcpy(weights, &vwscaled, 4*sizeof(*weights));
            joints += 4;
            weights += 4;
            ++daevcountitr;
        }
        assert(daevitr == daevend);
    }
    else
    {
        taa_LOG_ERROR("invalid skin vertex data");
    }
}
