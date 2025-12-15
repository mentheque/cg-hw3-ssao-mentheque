#pragma once 


// ! Do not effect shader side
#define __DIRECTIONAL_COUNT__ 3
#define __SPOT_COUNT__ 4

#define __LIGHT_MODEL_M__ lightModelManager<__DIRECTIONAL_COUNT__, __SPOT_COUNT__>

// Indexes of user controlled lights 
#define __USER_SPOT__ 3
#define __USER_DIR__ 2

// directional light model is offset from 0 by 
#define __DIR_LIGHT_OFFSET_ -70.0

// Time of one revolution of directional lights in ms
#define __ONE_REVOLUTION_PER_ 10000

#define __MORPH_MODEL_FILE__ ":/Models/blowfish2.glb"