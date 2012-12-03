#include "convert.h"
#include <taa/log.h>
#include <taa/quat.h>
#include <taa/scalar.h>
#include <assert.h>
#include <string.h>

//****************************************************************************
static void convert_node_bind_material(
    dae_COLLADA* collada,
    dae_bind_material_type* daebindmat,
    taa_scene* scene,
    taa_scenemesh* mesh,
    objmap* matmap)
{
    dae_bind_material_type_technique_common* daetech;
    daetech = daebindmat->el_technique_common;
    if(daetech != NULL)
    {
        dae_instance_material_type** daeinstmatitr;
        dae_instance_material_type** daeinstmatend;
        daeinstmatitr = daetech->el_instance_material.values;
        daeinstmatend = daeinstmatitr + daetech->el_instance_material.size;
        while(daeinstmatitr != daeinstmatend)
        {
            dae_instance_material_type* daeinstmat = *daeinstmatitr;
            int bindid = -1;
            int matid = -1;
            // find the mesh binding that matches the instance symbol name
            if(daeinstmat->at_symbol != NULL)
            {
                const char* symbol = *daeinstmat->at_symbol;
                if(symbol != NULL)
                {
                    bindid = taa_scenemesh_find_binding(mesh, symbol);
                }
            }
            // find the material that matches the instance target
            if(daeinstmat->at_target != NULL)
            {
                const char* uri = *daeinstmat->at_target;
                if(uri != NULL)
                {
                    dae_obj_ptr daematobj = daeu_search_uri(collada, uri);
                    if(daematobj != NULL)
                    {
                        matid = objmap_find(matmap, daematobj);
                    }
                }
            }
            if(bindid >= 0 && matid >= 0)
            {
                mesh->bindings[bindid].materialid = matid;
            }
            ++daeinstmatitr;
        }
    }
}

//****************************************************************************
static int convert_node_matrix(
    dae_matrix_type* daematrix,
    int parentnode,
    taa_scene* scene,
    objmap* nodemap)
{
    int nodeid;
    const char* name;
    taa_mat44 m;
    name = NULL;
    if(daematrix->at_sid != NULL)
    {
        name = *daematrix->at_sid;
    }
    if(name == NULL)
    {
        name = "";
    }
    taa_vec4_set(
        daematrix->data[ 0],
        daematrix->data[ 4],
        daematrix->data[ 8],
        daematrix->data[12],
        &m.x);
    taa_vec4_set(
        daematrix->data[ 1],
        daematrix->data[ 5],
        daematrix->data[ 9],
        daematrix->data[13],
        &m.y);
    taa_vec4_set(
        daematrix->data[ 2],
        daematrix->data[ 6],
        daematrix->data[10],
        daematrix->data[14],
        &m.z);
    taa_vec4_set(
        daematrix->data[ 3],
        daematrix->data[ 7],
        daematrix->data[11],
        daematrix->data[15],
        &m.w);
    nodeid = taa_scene_add_node(
        scene,
        name,
        taa_SCENENODE_TRANSFORM_MATRIX,
        parentnode);
    scene->nodes[nodeid].value.matrix = m;
    objmap_insert(nodemap, daematrix, nodeid);

    return nodeid;
}

//****************************************************************************
static int convert_node_rotate(
    dae_rotate_type* daerotate,
    int parentnode,
    taa_scene* scene,
    objmap* nodemap)
{
    int nodeid;
    const char* name;
    taa_vec4 v;
    name = NULL;
    if(daerotate->at_sid != NULL)
    {
        name = *daerotate->at_sid;
    }
    if(name == NULL)
    {
        name = "";
    }
    taa_vec4_set(
        daerotate->data[0],
        daerotate->data[1],
        daerotate->data[2],
        taa_radians(daerotate->data[3]),
        &v);
    nodeid = taa_scene_add_node(
        scene,
        name,
        taa_SCENENODE_TRANSFORM_ROTATE,
        parentnode);
    scene->nodes[nodeid].value.rotate = v;
    objmap_insert(nodemap, daerotate, nodeid);
    return nodeid;
}

//****************************************************************************
static int convert_node_scale(
    dae_scale_type* daescale,
    int parentnode,
    taa_scene* scene,
    objmap* nodemap)
{
    int nodeid;
    const char* name;
    taa_vec4 v;
    name = NULL;
    if(daescale->at_sid != NULL)
    {
        name = *daescale->at_sid;
    }
    if(name == NULL)
    {
        name = "";
    }
    taa_vec4_set(
        daescale->data[0],
        daescale->data[1],
        daescale->data[2],
        1.0f,
        &v);
    nodeid = taa_scene_add_node(
        scene,
        name,
        taa_SCENENODE_TRANSFORM_SCALE,
        parentnode);
    scene->nodes[nodeid].value.scale = v;
    objmap_insert(nodemap, daescale, nodeid);
    return nodeid;
}

//****************************************************************************
static int convert_node_translate(
    dae_translate_type* daetranslate,
    int parentnode,
    taa_scene* scene,
    objmap* nodemap)
{
    int nodeid;
    const char* name;
    taa_vec4 v;
    name = NULL;
    if(daetranslate->at_sid != NULL)
    {
        name = *daetranslate->at_sid;
    }
    if(name == NULL)
    {
        name = "";
    }
    taa_vec4_set(
        daetranslate->data[0],
        daetranslate->data[1],
        daetranslate->data[2],
        0.0f,
        &v);
    nodeid = taa_scene_add_node(
        scene,
        name,
        taa_SCENENODE_TRANSFORM_TRANSLATE,
        parentnode);
    scene->nodes[nodeid].value.translate = v;
    objmap_insert(nodemap, daetranslate, nodeid);
    return nodeid;
}


//****************************************************************************
void convert_node(
    dae_COLLADA* collada,
    dae_node_type* daenode,
    int parentnode,
    int skelid,
    int parentjoint,
    taa_scene* scene,
    objmap* nodemap)
{
    int32_t nodeid;
    int32_t jointid;
    dae_node_type** daechilditr;
    dae_node_type** daechildend;
    dae_obj_ptr daeel;
    const char* name;

    // convert transforms for node
    nodeid = parentnode;
    daeel = dae_get_first_element(daenode);
    while(daeel != NULL)
    {
        dae_obj_typeid daeeltype = dae_get_typeid(daeel);
        switch(daeeltype)
        {
        case dae_ID_LOOKAT_TYPE:
            // unsupported
            assert(0);
            break;
        case dae_ID_MATRIX_TYPE:
            nodeid = convert_node_matrix(
                (dae_matrix_type*) daeel,
                nodeid,
                scene,
                nodemap);
            break;
        case dae_ID_ROTATE_TYPE:
            nodeid = convert_node_rotate(
                (dae_rotate_type*) daeel,
                nodeid,
                scene,
                nodemap);
            break;
        case dae_ID_SCALE_TYPE:
            nodeid = convert_node_scale(
                (dae_scale_type*) daeel,
                nodeid,
                scene,
                nodemap);
            break;
        case dae_ID_SKEW_TYPE:
            // unsupported
            assert(0);
            break;
        case dae_ID_TRANSLATE_TYPE:
            nodeid = convert_node_translate(
                (dae_translate_type*) daeel,
                nodeid,
                scene,
                nodemap);
            break;
        default:
            break;
        }
        daeel = dae_get_next(daeel);
    }

    // create the node
    name = NULL;
    if(daenode->at_name != NULL)
    {
        name = *daenode->at_name;
    }
    if(name == NULL && daenode->at_id != NULL)
    {
        name = *daenode->at_id;
    }
    if(name == NULL && daenode->at_sid != NULL)
    {
        name = *daenode->at_sid;
    }
    if(name == NULL)
    {
        name = "";
    }

    // create the node
    jointid = parentjoint;
    parentnode = nodeid;
    nodeid = -1;
    if(daenode->at_type != NULL)
    {
        // if the node is a joint, add it to the current skeleton
        const char* type = *daenode->at_type;
        if(type != NULL && !strcmp(type, "JOINT"))
        {
            // the node is a joint
            nodeid = parentnode;
            if(skelid < 0)
            {
                // make sure it's within a skeleton
                const char* skelname = NULL;
                if(skelname == NULL && daenode->at_name != NULL)
                {
                    skelname = *daenode->at_name;
                }
                if(daenode->at_id != NULL)
                {
                    skelname = *daenode->at_id;
                }
                if(skelname == NULL && daenode->at_sid != NULL)
                {
                    skelname = *daenode->at_sid;
                }
                if(skelname == NULL)
                {
                    skelname = "";
                }
                skelid = taa_scene_add_skeleton(scene, skelname);
                nodeid = taa_scene_add_node(
                    scene,
                    skelname,
                    taa_SCENENODE_REF_SKEL,
                    nodeid);
                scene->nodes[nodeid].value.skelid = skelid;
            }
            // associate a joint with it
            nodeid = taa_scene_add_node(
                scene,
                name,
                taa_SCENENODE_REF_JOINT,
                nodeid);
            jointid = taa_sceneskel_add_joint(
                scene->skeletons + skelid,
                parentjoint,
                nodeid);
            scene->nodes[nodeid].value.jointid = jointid;
        }
    }
    if(nodeid == -1)
    {
        // if the node is not a joint, insert an empty one
        nodeid = taa_scene_add_node(
            scene,
            name,
            taa_SCENENODE_EMPTY,
            parentnode);
    }
    objmap_insert(nodemap, daenode, nodeid);

    daechilditr = daenode->el_node.values;
    daechildend = daechilditr + daenode->el_node.size;
    while(daechilditr != daechildend)
    {
        // recurse into any child nodes
        convert_node(
            collada,
            *daechilditr,
            nodeid,
            skelid,
            jointid,
            scene,
            nodemap);
        ++daechilditr;
    }

    // TODO: handle node instances
}

//****************************************************************************
void convert_node_bindings(
    dae_COLLADA* collada,
    dae_node_type* daenode,
    taa_scene* scene,
    objmap* matmap,
    objmap* geommap,
    objmap* nodemap)
{
    dae_node_type** daechilditr;
    dae_node_type** daechildend;
    dae_instance_geometry_type** daeinstgeomitr;
    dae_instance_geometry_type** daeinstgeomend;
    dae_instance_controller_type** daeinstctrlitr;
    dae_instance_controller_type** daeinstctrlend;
    int nodeid;

    // find the scene node associated with the collada node
    nodeid = objmap_find(nodemap, daenode);
    assert(nodeid >= 0); // should never fail

    // add references to any geometry
    daeinstgeomitr = daenode->el_instance_geometry.values;
    daeinstgeomend = daeinstgeomitr + daenode->el_instance_geometry.size;
    while(daeinstgeomitr != daeinstgeomend)
    {
        dae_instance_geometry_type* daeinstgeom = *daeinstgeomitr;
        dae_obj_ptr daegeomobj = NULL;
        int meshid = -1;
        // get the uri for the geometry and search for it within the document
        if(daeinstgeom->at_url != NULL)
        {
            const char* uri = *daeinstgeom->at_url;
            if(uri != NULL)
            {
               daegeomobj = daeu_search_uri(collada, uri);
            }
        }
        // find the scene mesh that was created from the collada geometry
        if(daegeomobj != NULL)
        {
            meshid = objmap_find(geommap, daegeomobj);
        }
        else
        {
            taa_LOG_ERROR("geometry not found");
        }
        // bind it to the node
        if(nodeid >= 0 && meshid >= 0)
        {
            int refnodeid;
            if(daeinstgeom->el_bind_material != NULL)
            {
                convert_node_bind_material(
                    collada,
                    daeinstgeom->el_bind_material,
                    scene,
                    scene->meshes + meshid,
                    matmap);
            }
            refnodeid = taa_scene_add_node(
                scene,
                "",
                taa_SCENENODE_REF_MESH,
                nodeid);
            scene->nodes[refnodeid].value.meshid = meshid;
        }
        else
        {
            taa_LOG_ERROR("mesh not found");
        }
        ++daeinstgeomitr;
    }

    // add references to any skin controllers
    daeinstctrlitr = daenode->el_instance_controller.values;
    daeinstctrlend = daeinstctrlitr + daenode->el_instance_controller.size;
    while(daeinstctrlitr != daeinstctrlend)
    {
        dae_instance_controller_type* daeinstctrl = *daeinstctrlitr;
        dae_skin_type* daeskin = NULL;
        // attempt to find the skin controller
        if(daeinstctrl->at_url != NULL)
        {
            const char* uri = *daeinstctrl->at_url;
            if(uri != NULL)
            {
               dae_obj_ptr daectrlobj = daeu_search_uri(collada, uri);
               if(daectrlobj != NULL)
               {
                   if(dae_get_typeid(daectrlobj) == dae_ID_CONTROLLER_TYPE)
                   {
                       daeskin = ((dae_controller_type*) daectrlobj)->el_skin;
                   }
               }
            }
        }
        if(daeskin != NULL)
        {
            int meshid = -1;
            // attempt to find the mesh associated with the skin
            if(daeskin->at_source != NULL)
            {
                const char* uri = *daeskin->at_source;
                if(uri != NULL)
                {
                    dae_obj_ptr geomobj = daeu_search_uri(collada, uri);
                    if(geomobj != NULL)
                    {
                        meshid = objmap_find(geommap, geomobj);
                    }
                }
                else
                {
                    taa_LOG_ERROR("geometry not found");
                }
            }
            // TODO: add support for skeleton element
            if(nodeid >= 0 && meshid >= 0)
            {
                taa_scenemesh* mesh = scene->meshes + meshid;
                int refnodeid;
                // convert skinning data and assign it to the mesh
                convert_skin(collada, daeskin, scene, mesh, nodemap);

                if(daeinstctrl->el_bind_material != NULL)
                {
                    convert_node_bind_material(
                        collada,
                        daeinstctrl->el_bind_material,
                        scene,
                        mesh,
                        matmap);
                }

                // bind the mesh to the node
                refnodeid = taa_scene_add_node(
                    scene,
                    "",
                    taa_SCENENODE_REF_MESH,
                    nodeid);
                scene->nodes[refnodeid].value.meshid = meshid;
            }
            else
            {
                taa_LOG_ERROR("mesh not found");
            }
        }
        ++daeinstctrlitr;
    }

    daechilditr = daenode->el_node.values;
    daechildend = daechilditr + daenode->el_node.size;
    while(daechilditr != daechildend)
    {
        // recurse into any child nodes
        convert_node_bindings(collada,*daechilditr,scene,matmap,geommap,nodemap);
        ++daechilditr;
    }

    // TODO: handle node instances
}
