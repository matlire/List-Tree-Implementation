#ifndef TYPES_H
#define TYPES_H

#define begin do {
#define end } while (0)

#define unused (void)

typedef enum 
{
    OK          = 0,
    ERR_BAD_ARG = 1,
    ERR_CORRUPT = 2,
    ERR_ALLOC   = 3,
} err_t;

#endif
