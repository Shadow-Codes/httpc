#include "http_error_codes.h"
#include <stddef.h>

const codemap_t code_registery[] = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {431, "Request Header Fields Too Large"},
    {500, "Internal Server Error"},
    {505, "HTTP Version Not Supported"},
};

const size_t registery_len = sizeof(code_registery) / sizeof(code_registery[0]);
