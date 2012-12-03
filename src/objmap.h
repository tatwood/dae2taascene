#ifndef OBJMAP_H_
#define OBJMAP_H_

#include <dae.h>

typedef struct objmap_s objmap;

struct objmap_s
{
    dae_obj_ptr* values;
    size_t size;
};

//****************************************************************************
// functions

void objmap_create(
    objmap* map);

void objmap_destroy(
    objmap* map);

int objmap_find(
    objmap* map,
    dae_obj_ptr obj);

void objmap_insert(
    objmap* map,
    dae_obj_ptr obj,
    int id);

#endif // OBJMAP_H_
