#include "convert.h"

//****************************************************************************
static void convert_geometry_source(
    dae_geometry_type* geom,
    const char* semantic,
    uint32_t offset,
    uint32_t set,
    dae_source_type* daesource,
    taa_scenemesh* mesh)
{
    void* data;
    int32_t datasize;
    taa_scenemesh_usage usage;
    taa_scenemesh_valuetype type;
    int32_t components;
    uint32_t count;
    taa_scenemesh_stream* vsitr;
    taa_scenemesh_stream* vsend;

    data = NULL;
    datasize = 0;
    usage = taa_SCENEMESH_USAGE_MERGED;
    type = taa_SCENEMESH_VALUE_FLOAT32;
    components = 0;
    count = 0;

    if (daesource->el_technique_common != NULL)
    {
        dae_accessor_type* daeaccessor;
        daeaccessor = daesource->el_technique_common->el_accessor;
        if(daeaccessor != NULL)
        {
            if(daeaccessor->at_stride != NULL)
            {
                components = *daeaccessor->at_stride;
            }
            if(components == 0)
            {
                components = daeaccessor->el_param.size;
            }
            if(daeaccessor->at_count != NULL)
            {
                count = *daeaccessor->at_count;
            }
        }
    }
    if(daesource->el_float_array != NULL)
    {
        data = daesource->el_float_array->data.values;
        datasize = sizeof(float);
        type = taa_SCENEMESH_VALUE_FLOAT32;
        if(count == 0)
        {
            count = daesource->el_float_array->data.size / components;
        }
    }
    else if(daesource->el_int_array != NULL)
    {
        data = daesource->el_int_array->data.values;
        datasize = sizeof(int);
        type = taa_SCENEMESH_VALUE_INT32;
        if(count == 0)
        {
            count = daesource->el_int_array->data.size / components;
        }
    }

    if(semantic != NULL)
    {
        if(!strcmp(semantic, "BINORMAL"))
        {
            usage = taa_SCENEMESH_USAGE_BINORMAL;
        }
        else if(!strcmp(semantic, "COLOR"))
        {
            usage = taa_SCENEMESH_USAGE_COLOR;
        }
        else if(!strcmp(semantic, "NORMAL"))
        {
            usage = taa_SCENEMESH_USAGE_NORMAL;
        }
        else if(!strcmp(semantic, "POSITION"))
        {
            usage = taa_SCENEMESH_USAGE_POSITION;
        }
        else if(!strcmp(semantic, "TANGENT"))
        {
            usage = taa_SCENEMESH_USAGE_TANGENT;
        }
        else if(!strcmp(semantic, "TEXCOORD"))
        {
            usage = taa_SCENEMESH_USAGE_TEXCOORD;
        }
        else if(!strcmp(semantic, "UV"))
        {
            usage = taa_SCENEMESH_USAGE_TEXCOORD;
        }

        vsitr = mesh->vertexstreams;
        vsend = vsitr + mesh->numstreams;
        while(vsitr != vsend)
        {
            if(vsitr->indexmapping == offset)
            {
                // if the index offsets match, make sure they have the
                // same number of vertices
                assert(count==vsitr->numvertices);
            }
            if(!strcmp(vsitr->name, semantic) && set == vsitr->set)
            {
                // the vertex stream already exists.
                assert(offset == vsitr->indexmapping);
                break;
            }
            ++vsitr;
        }

        if(vsitr == vsend)
        {
            // if the vertex stream did not already exist, add a new one
            int stride = components * datasize;
            taa_scenemesh_add_stream(
                mesh,
                semantic,
                usage,
                set,
                type,
                components,
                stride,
                offset,
                count,
                data);
        }
    }
}

//****************************************************************************
static void convert_geometry_vertices(
    dae_geometry_type* daegeom,
    dae_input_local_offset_type* daeinput,
    dae_vertices_type* daeverts,
    taa_scenemesh* mesh)
{
    dae_mesh_type* daemesh = daegeom->el_mesh;
    dae_input_local_type** daeinputitr = daeverts->el_input.values;
    dae_input_local_type** daeinputend = daeinputitr+daeverts->el_input.size;
    while(daeinputitr != daeinputend)
    {
        dae_input_local_type* daeinputlocal = *daeinputitr;
        dae_source_type* daesource = NULL;
        if(daeinputlocal->at_source != NULL)
        {
            dae_obj_ptr daesourceobj;
            daesourceobj = daeu_search_uri(daemesh,*daeinputlocal->at_source);
            if(daesourceobj != NULL)
            {
                if(dae_get_typeid(daesourceobj) == dae_ID_SOURCE_TYPE)
                {
                    daesource = (dae_source_type*) daesourceobj;
                }
            }
        }
        if(daesource != NULL)
        {
            const char* semantic = "";
            uint32_t offset = 0;
            uint32_t set = 0;
            if(daeinputlocal->at_semantic != NULL)
            {
                semantic = *daeinputlocal->at_semantic;
            }
            else if(daeinput->at_semantic != NULL)
            {
                semantic = *daeinput->at_semantic;
            }
            if(daeinput->at_offset != NULL)
            {
                offset = *daeinput->at_offset;
            }
            if(daeinput->at_set != NULL)
            {
                set = *daeinput->at_set;
            }
            convert_geometry_source(
                daegeom,
                semantic,
                offset,
                set,
                daesource,
                mesh);
        }
        ++daeinputitr;
    }
}

//****************************************************************************
static void convert_geometry_input(
    dae_geometry_type* daegeom,
    dae_input_local_offset_type* daeinput,
    taa_scenemesh* mesh)
{
    dae_mesh_type* daemesh = daegeom->el_mesh;
    dae_obj_ptr daesourceobj = NULL;
    if(daeinput->at_source != NULL)
    {
        const char* daeuri = *daeinput->at_source;
        if(daeuri != NULL)
        {
            daesourceobj = daeu_search_uri(daemesh, daeuri);
        }
    }
    if(daesourceobj != NULL)
    {
        dae_obj_typeid daesourcetype = dae_get_typeid(daesourceobj);
        if(daesourcetype == dae_ID_SOURCE_TYPE)
        {
            dae_source_type* daesource = (dae_source_type*) daesourceobj;
            const char* semantic = "";
            uint32_t offset = 0;
            uint32_t set = 0;
            if(daeinput->at_semantic != NULL)
            {
                semantic = *daeinput->at_semantic;
            }
            if(daeinput->at_offset != NULL)
            {
                offset = *daeinput->at_offset;
            }
            if(daeinput->at_set != NULL)
            {
                set = *daeinput->at_set;
            }
            convert_geometry_source(
                daegeom,
                semantic,
                offset,
                set,
                daesource,
                mesh);
        }
        else if(daesourcetype == dae_ID_VERTICES_TYPE)
        {
            dae_vertices_type* daeverts = (dae_vertices_type*) daesourceobj;
            convert_geometry_vertices(
                daegeom,
                daeinput,
                daeverts,
                mesh);
        }
    }
}

//****************************************************************************
static void convert_geometry_polygons(
    dae_COLLADA* collada,
    dae_geometry_type* daegeom,
    dae_polygons_type* daepolys,
    taa_scenemesh* mesh)
{
    dae_input_local_offset_type** daeinputitr;
    dae_input_local_offset_type** daeinputend;
    dae_p_type** daepitr;
    dae_p_type** daepend;
    const char* bindname;
    // convert inputs
    daeinputitr = daepolys->el_input.values;
    daeinputend = daeinputitr + daepolys->el_input.size;
    while(daeinputitr != daeinputend)
    {
        convert_geometry_input(daegeom, *daeinputitr, mesh);
        ++daeinputitr;
    }
    // find material binding
    bindname = NULL;
    if(daepolys->at_material != NULL)
    {
        bindname = *daepolys->at_material;
    }
    if(bindname == NULL)
    {
        bindname = "";
    }
    // convert faces
    taa_scenemesh_begin_binding(mesh, bindname, -1);
    daepitr = daepolys->el_p.values;
    daepend = daepitr + daepolys->el_p.size;
    while(daepitr != daepend)
    {
        dae_p_type* daep = *daepitr;
        uint32_t numverts = daep->data.size / mesh->indexsize;
        taa_scenemesh_add_face(
            mesh,
            daep->data.values,
            daep->data.size,
            numverts);
        ++daepitr;
    }
    taa_scenemesh_end_binding(mesh);
}

//****************************************************************************
static void convert_geometry_polylist(
    dae_COLLADA* collada,
    dae_geometry_type* daegeom,
    dae_polylist_type* daepolylist,
    taa_scenemesh* mesh)
{
    dae_input_local_offset_type** daeinputitr;
    dae_input_local_offset_type** daeinputend;
    const char* bindname;

    // convert inputs
    daeinputitr = daepolylist->el_input.values;
    daeinputend = daeinputitr + daepolylist->el_input.size;
    while(daeinputitr != daeinputend)
    {
        convert_geometry_input(daegeom, *daeinputitr, mesh);
        ++daeinputitr;
    }
    // find material binding
    bindname = NULL;
    if(daepolylist->at_material != NULL)
    {
        bindname = *daepolylist->at_material;
    }
    if(bindname == NULL)
    {
        bindname = "";
    }
    // convert faces
    taa_scenemesh_begin_binding(mesh, bindname, -1);
    if(daepolylist->el_vcount != NULL && daepolylist->el_p != NULL)
    {
        const unsigned int* daevcountitr;
        const unsigned int* daevcountend;
        const unsigned int* daepitr;
        const unsigned int* daepend;
        daevcountitr = daepolylist->el_vcount->data.values; 
        daevcountend = daevcountitr + daepolylist->el_vcount->data.size;
        daepitr = daepolylist->el_p->data.values;
        daepend = daepitr + daepolylist->el_p->data.size;
        while(daevcountitr != daevcountend)
        {
            unsigned int numverts = *daevcountitr;
            unsigned int numindices = numverts*mesh->indexsize;
            if(daepitr + numindices <= daepend)
            {
                taa_scenemesh_add_face(
                    mesh,
                    daepitr,
                    numindices,
                    numverts);
                daepitr += numindices;
            }
            else
            {
                taa_LOG_ERROR("mesh index overflow");
            }
            ++daevcountitr;
        }
        assert(daepitr == daepend);
    }
    else
    {
        taa_LOG_ERROR("invalid mesh polylist");
    }
    taa_scenemesh_end_binding(mesh);
}

//****************************************************************************
void convert_geometry(
    dae_COLLADA* collada,
    dae_geometry_type* daegeom,
    taa_scene* scene,
    objmap* geommap)
{
    dae_mesh_type* daemesh = daegeom->el_mesh;
    if(daemesh != NULL)
    {
        int meshid;
        const char* name;
        taa_scenemesh* mesh;
        dae_polygons_type** daepolysitr;
        dae_polygons_type** daepolysend;
        dae_polylist_type** daepolylistitr;
        dae_polylist_type** daepolylistend;
        // find the name
        name = NULL;
        if(daegeom->at_name != NULL)
        {
            name = *daegeom->at_name;
        }
        if(name == NULL && daegeom->at_id != NULL)
        {
            name = *daegeom->at_id;
        }
        if(name == NULL)
        {
            name = "";
        }
        // create the mesh
        meshid = taa_scene_add_mesh(scene, name);
        mesh = scene->meshes + meshid;
        // polygons
        daepolysitr = daemesh->el_polygons.values;
        daepolysend = daepolysitr + daemesh->el_polygons.size;
        while(daepolysitr != daepolysend)
        {
            convert_geometry_polygons(
                collada,
                daegeom,
                *daepolysitr,
                mesh);
            ++daepolysitr;
        }
        // polylist
        daepolylistitr = daemesh->el_polylist.values;
        daepolylistend = daepolylistitr + daemesh->el_polylist.size;
        while(daepolylistitr != daepolylistend)
        {
            convert_geometry_polylist(
                collada,
                daegeom,
                *daepolylistitr,
                mesh);
            ++daepolylistitr;
        }
        // insert the geometry into the conversion map
        objmap_insert(geommap, daegeom, meshid);
    }
}
