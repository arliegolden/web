#ifndef MIME_TYPES_H
#define MIME_TYPES_H

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

const char* get_mime_type(const char *path);

#endif