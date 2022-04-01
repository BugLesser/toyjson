#pragma once

#include <ctype.h>

typedef enum JsonObjectType {
    JOT_INVALID = 0,
    JOT_NULL,
    JOT_BOOL,
    JOT_NUMBER,
    JOT_STRING,
    JOT_ARRAY,
    JOT_OBJECT
} JsonObjectType;

typedef struct JsonObject {
    JsonObjectType type;
    char* name;
    struct JsonObject* prev;
    struct JsonObject* next;
    union {
        struct JsonObject* vChild; /* array and object child head */
        int vBool;
        double vNumber;
        char* vString;
    };
    
} JsonObject;

typedef struct JsonParser {
    const char* content;
    size_t length;
    size_t index;
} JsonParser;


void jsonparser_init(JsonParser* parser, char* json);
JsonObject* jsonparser_parse_content(char* json);
JsonObject* jsonparser_parse_file(const char* filename);

char* jsonobject_format(JsonObject* object);
