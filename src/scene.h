#ifndef CARDS_SCENE_H
#define CARDS_SCENE_H
#include "model.h"
typedef struct { Pile stock,waste,foundations[4],tableau[7]; } Scene;
/* Preview fixture, deliberately independent of Klondike legality. */
void scene_create(Scene *scene,uint32_t seed,bool stress);
#endif
