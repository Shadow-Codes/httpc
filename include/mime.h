#ifndef MIME_H
#define MIME_H

typedef enum MimeType {
    TYPE_HTML,
    TYPE_CSS,
    TYPE_UNKNOWN,
} mimetype_t;

typedef struct mime_mapping {
    const char *extension;
    mimetype_t type;
} mime_mapping_t;


void get_extension(char *request_target, char *extension);
mimetype_t get_mimetype(char *extension);
const char *get_content_type(mimetype_t type);

#endif
