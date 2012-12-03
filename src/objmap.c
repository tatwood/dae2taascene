#include "objmap.h"
#include <stdlib.h>
#include <string.h>

//****************************************************************************
void objmap_create(
    objmap* map)
{
    memset(map, 0, sizeof(*map));
}

//****************************************************************************
void objmap_destroy(
    objmap* map)
{
    free(map->values);
}

//****************************************************************************
int objmap_find(
    objmap* map,
    dae_obj_ptr obj)
{
    int result = -1;
    dae_obj_ptr* itr = map->values;
    dae_obj_ptr* end = itr + map->size;
    while(itr != end)
    {
        if(*itr == obj)
        {
            result = (int)(ptrdiff_t) (itr - map->values);
            break;
        }
        ++itr;
    }
    return result;
}

//****************************************************************************
void objmap_insert(
    objmap* map,
    dae_obj_ptr obj,
    int id)
{
    if(((size_t) id) >= map->size)
    {
        int i;
        map->values = (dae_obj_ptr*) realloc(map->values,(id+1)*sizeof(obj));
        for(i = map->size; i < id; ++i)
        {
            map->values[i] = NULL;
        }
        map->size = id + 1;
    }
    map->values[id] = obj;
}
