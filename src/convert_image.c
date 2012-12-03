#include "convert.h"
#include "tga.h"
#include <taa/path.h>
#include <taa/log.h>
#include <taa/uri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//****************************************************************************
int convert_image(
    const char* colladapath,
    dae_COLLADA* collada,
    dae_image_type* daeimage,
    taa_scene* scene,
    objmap* imagemap)
{
    int32_t err = 0;
    dae_image_type_init_from* daeinitfrom = daeimage->el_init_from;
    char path[taa_PATH_SIZE];
    const char* name;
    void* data;
    size_t size;
    data = NULL;
    size = 0;
    path[0] = '\0';
    // determine name
    name = NULL;
    if(daeimage->at_name != NULL)
    {
        name = *daeimage->at_name;
    }
    if(name == NULL && daeimage->at_id != NULL)
    {
        name = *daeimage->at_id;
    }
    if(name == NULL)
    {
        name = "";
    }
    // determine if image is external file
    if(daeinitfrom != NULL)
    {
        if(daeinitfrom->el_ref)
        {
            char* uri = *daeinitfrom->el_ref;
            if(uri != NULL)
            {
                char urischeme[16];
                char uripath[taa_PATH_SIZE];
                taa_uri_get_scheme(uri, urischeme, sizeof(urischeme));
                if(urischeme[0] != '\0')
                {
                    // valid uri
                    taa_uri_get_path(uri, uripath, sizeof(uripath));
                    taa_path_set(path, sizeof(path), uripath);
                }
                else
                {
                    // assume it's just a file path
                    taa_path_set(path, sizeof(path), uri);
                }
            }
        }
    }
    // load external file
    if(*path != '\0')
    {
        FILE* fp = fopen(path, "rb");
        if(fp == NULL)
        {
            // if could not find image relative to cwd,
            // try making it relative to document path
            char relpath[taa_PATH_SIZE];
            taa_path_get_dir(colladapath, relpath, sizeof(relpath));
            taa_path_append(relpath, sizeof(relpath), path);
            fp = fopen(relpath, "rb");
        }
        if(fp == NULL)
        {
            // TODO: reattempt using local dir
        }
        if(fp != NULL)
        {
            fseek(fp, 0, SEEK_END);
            size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            data = malloc(size);
            if(fread(data, 1, size, fp) != size)
            {
                taa_LOG_WARN("error reading image file %s", path);
                free(data);
                size = 0;
                err = -1;
            }
            fclose(fp);
        }
        else
        {
            taa_LOG_WARN("could not open image file %s", path);
            err = -1;
        }
    }
    // process image
    if(data != NULL)
    {
        tga_header tga;
        size_t imageoff;
        if(err == 0)
        {
            if(size > tga_HEADER_SIZE)
            {
                tga_read(data, &tga, &imageoff);
            }
            else
            {
                taa_LOG_WARN("invalid image size %s", path);
                err = -1;
            }
        }
        if(err == 0)
        {
            if(
                tga.imagetype!=tga_TYPE_TRUECOLOR &&
                tga.imagetype!=tga_TYPE_GREY)
            {
                taa_LOG_WARN("unsupported image color type for %s", path);
                err = -1; // only support truecolor uncompressed or grey scale
            }
            if(tga.colourmaptype != 0 || tga.colourmaplength != 0)
            {
                taa_LOG_WARN("colour maps not supported for %s", path);
                err = -1; // do not support colour maps
            }
            if((tga.descriptor & 0xC0) != 0)
            {
                taa_LOG_WARN("interleaved data not supported for %s", path);
                err = -1; // do not support interleaved data
            }        
            if(imageoff + tga.width*tga.height*tga.bitsperpixel/8 > size)
            {
                taa_LOG_WARN("image buffer overflow for %s", path);
                err = -1; // check for buffer overflow
            }
        }
        if(err == 0)
        {
            taa_scenetexture* tex;
            taa_scenetexture_origin origin;
            taa_scenetexture_format format;
            int32_t texid;
            origin = taa_SCENETEXTURE_BOTTOMLEFT;
            if(tga_GET_VFLIP(&tga))
            {
                origin = taa_SCENETEXTURE_TOPLEFT;
            }
            switch(tga.bitsperpixel)
            {
            case  8:
                format = taa_SCENETEXTURE_LUM8;
                break;
            case 24:
                format = taa_SCENETEXTURE_BGR8;
                break;
            case 32:
                format = taa_SCENETEXTURE_BGRA8;
                break;
            default:
                taa_LOG_WARN("invalid pixel format for image %s", path);
                err = -1;
                break;
            }
            if(err == 0)
            {
                // add the texture to the scene
                texid = taa_scene_add_texture(
                    scene,
                    name,
                    path,
                    origin);
                tex = scene->textures + texid;
                taa_scenetexture_resize(
                    tex,
                    1,
                    tga.width,
                    tga.height,
                    1,
                    format);
                memcpy(
                    tex->images[0],
                    ((unsigned char*) data) + imageoff,
                    tga.width * tga.height * tga.bitsperpixel/8);
                // insert the image into the conversion map
                objmap_insert(imagemap, daeimage, texid);
            }
        }
        free(data);
    }
    return err;
}

