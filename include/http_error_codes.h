#ifndef HTTP_ERROR_CODES
#define HTTP_ERROR_CODES

#include <stddef.h>

typedef struct codemap {
    int code;
    const char *phrase;
} codemap_t;

extern const codemap_t code_registery[];
extern const size_t registery_len;

#endif
