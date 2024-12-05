#include "mime_types.h"
#include <string.h>
#include <stdio.h>
#include <strings.h>

static const mime_map mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".rar", "application/x-rar-compressed"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {NULL, NULL}
};

const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) {
        return "application/octet-stream";
    }

    for (int i = 0; mime_types[i].extension != NULL; i++) {
        if (strcasecmp(ext, mime_types[i].extension) == 0) {
            return mime_types[i].mime_type;
        }
    }
    
    return "application/octet-stream";
}