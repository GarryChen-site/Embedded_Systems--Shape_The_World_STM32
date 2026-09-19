// game_types.h — Lab 15 Space Invaders, HAL version
//
// Shared type definition for game sprites.


#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#include <stdint.h>

// Game sprite object — mirrors STyp in SpaceInvaders_stm32.c
typedef struct {
    uint32_t        x;
    uint32_t        maxpoint[2];
    uint32_t        y;
    const uint8_t  *image[4];
    int32_t         life;
    uint32_t        deathCounter;
} STyp;

#endif /* GAME_TYPES_H */
