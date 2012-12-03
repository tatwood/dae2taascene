#if defined(_DEBUG) && defined(_MSC_FULL_VER)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "convert.h"
#include <taa/scene.h>
#include <taa/scenefile.h>
#include <taa/path.h>
#include <expat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAIN_USAGE \
    "Required arguments\n" \
    "  -dae                Path to input collada file\n" \
    "  -o [FILE]           Path to outputtaascene file\n" \
    "\n" \
    "Options\n" \
    "  -up [Y|Z]           Up axis for exported obj file\n"

typedef struct main_args_s main_args;

struct main_args_s
{
    taa_scene_upaxis upaxis;
    const char* dae;
    const char* out;
};

static int main_parse_args(
    int argc,
    char* argv[],
    main_args* args_out)
{
    int err = 0;
    char** argitr = argv;
    char** argend = argitr + argc;
    // initialize output
    args_out->upaxis = taa_SCENE_Y_UP;
    args_out->dae = NULL;
    args_out->out   = NULL;
    // skip past argv[0]
    ++argitr; 
    // loop
    while(argitr != argend)
    {
        const char* arg = *argitr;
        if(!strcmp(arg, "-dae"))
        {
            if(argitr+1 == argend)
            {
                err = -1;
                break;
            }
            args_out->dae = argitr[1];
            argitr += 2;
        }
        else if(!strcmp(arg, "-o"))
        {
            if(argitr+1 == argend)
            {
                err = -1;
                break;
            }
            args_out->out = argitr[1];
            argitr += 2;
        }
        else if(!strcmp(arg, "-up"))
        {
            if(argitr+1 == argend)
            {
                err = -1;
                break;
            }
            if(toupper(argitr[1][0]) == 'Z')
            {
                args_out->upaxis = taa_SCENE_Z_UP;
            }
            argitr += 2;
        }
        else
        {
            err = -1;
            break;
        }
    }
    if(args_out->dae==NULL || args_out->out==NULL)
    {
        err = -1;
    }
    return err;
}


int main(int argc, char* argv[])
{
    int err = 0;
    XML_Parser expat = XML_ParserCreate(NULL);
    main_args args;
    dae_COLLADA* collada = NULL;
    FILE* fp = NULL;
    void* data = NULL;
    size_t size = 0;
    taa_scene scene;

    err = main_parse_args(argc, argv, &args);
    if(err != 0)
    {
        puts(MAIN_USAGE);
    }
    taa_scene_create(&scene, taa_SCENE_Y_UP);
    if(err == 0)
    {
        // open input file
        fp = fopen(args.dae, "rb");
        if(fp == NULL)
        {
            printf("could not open input file %s\n", args.dae);
            err = -1;
        }
    }
    if(err == 0)
    {
        // get allocate file buffer
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        data = XML_GetBuffer(expat, size);
        if(data == NULL)
        {
            printf("could not allocate memory of size %u", (unsigned) size);
            err = -1;
        }
    }
    if(err == 0)
    {
        // read file
        if(fread(data, 1, size, fp) != size)
        {
            printf("error reading file\n");
            err = -1;
        }
    }
    if(fp != NULL)
    {
        fclose(fp);
    }
    if(err == 0)
    {
        // parse the dae data
        daeu_xml_parser parser;
        collada = dae_create();
        daeu_xml_create(collada, &parser);
        XML_SetElementHandler(
            expat,
            daeu_xml_startelement,
            daeu_xml_endelement);
        XML_SetCharacterDataHandler(expat, daeu_xml_chardata);
        XML_SetUserData(expat, parser);
        if(XML_ParseBuffer(expat, size, 1) != XML_STATUS_OK)
        {
            printf("error parsing xml\n");
            err = -1;
        }
        daeu_xml_destroy(parser);
    }
    XML_ParserFree(expat);
    if(err == 0)
    {
        // convert to taa_scene
        convert_collada(args.dae, collada, &scene);
    }
    if(collada != NULL)
    {
        dae_destroy(collada);
    }
    if(err == 0)
    {
        // convert up axis
        taa_scene_convert_upaxis(&scene, args.upaxis);
    }
    if(err == 0)
    {
        // open output file
        fp = fopen(args.out, "wb");
        if(fp == NULL)
        {
            printf("could not open output file %s\n", args.out);
            err = -1;
        }
    }
    if(err == 0)
    {
        // export
        taa_filestream outfs;
        taa_filestream_create(fp, 1024 * 1024, taa_FILESTREAM_WRITE, &outfs);
        taa_scenefile_serialize(&scene, &outfs);
        taa_filestream_destroy(&outfs);
        fclose(fp);
    }
    taa_scene_destroy(&scene);

#if defined(_DEBUG) && defined(_MSC_FULL_VER)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtCheckMemory();
    _CrtDumpMemoryLeaks();
#endif
    return err;
}
