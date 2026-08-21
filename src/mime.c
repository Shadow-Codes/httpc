#include "mime.h"
#include <string.h>

void get_extension(char *request_target, char *extension) {
    char *start_of_extension = strchr(request_target, '.');
    if (start_of_extension == NULL) {
        extension[0] = '\0';
        return;
    }
    char *end_of_extension = strchr(start_of_extension + 1, '\0'); 
    if (end_of_extension == NULL) {
        extension[0] = '\0';
        return;
    }
    memcpy(extension, start_of_extension + 1, end_of_extension - (start_of_extension + 1));
    extension[end_of_extension - (start_of_extension + 1)] = '\0';
}

static const mime_mapping_t mime_table[] = {
    {"html", TYPE_HTML},
    {"css", TYPE_CSS},
    {"", TYPE_UNKNOWN},
};

mimetype_t get_mimetype(char *extension) {
    int number_of_elements_mime_table = sizeof(mime_table) / sizeof(mime_table[0]);

    for (int i = 0; i < number_of_elements_mime_table; i++) {
        if (strcmp(mime_table[i].extension, extension) == 0) {
            return mime_table[i].type;
        }
    }
    return TYPE_UNKNOWN;
}

const char *get_content_type(mimetype_t type) {
    switch (type) {
        case TYPE_HTML:
            return "text/html";
        case TYPE_CSS:
            return "text/css";
        case TYPE_UNKNOWN:
        default:
            return "application/octet-stream";
    }
}
