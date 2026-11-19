#ifndef CARDS_CONTROLS_H
#define CARDS_CONTROLS_H
/* Platform-neutral logical buttons, reusable by other game modules. */
typedef enum {
    INPUT_LEFT, INPUT_RIGHT, INPUT_UP, INPUT_DOWN,
    INPUT_A, INPUT_B, INPUT_X, INPUT_Y,
    INPUT_L1, INPUT_R1, INPUT_L2, INPUT_R2, INPUT_START, INPUT_SELECT,
    INPUT_COUNT, INPUT_NONE
} InputAction;
#endif
