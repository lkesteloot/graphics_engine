#ifndef _DLOPCODE_H_
#define _DLOPCODE_H_

typedef enum DLOpcode {
    DL_VERTEX,
    DL_CALL,
    DL_RETURN,
    DL_PUSH,
    DL_POP,
    DL_LOAD,
    DL_MULT,
    DL_LIGHT,
    DL_SET,
    DL_RESET,
    DL_COLOR,
    DL_NORMAL
} DLOpcode;

#endif /* _DLOPCODE_H_ */
