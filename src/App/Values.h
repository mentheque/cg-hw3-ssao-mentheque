#pragma once 


// ! Do not effect shader side
#define __DIRECTIONAL_COUNT__ 3
#define __SPOT_COUNT__ 4

#define __LIGHT_MODEL_M__ lightModelManager<__DIRECTIONAL_COUNT__, __SPOT_COUNT__>

#define __USER_SPOT__ 3
#define __USER_DIR__ 2

#define __DIR_LIGHT_OFFSET_ -70.0