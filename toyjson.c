#include "toyjson.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define DEFAULT_STRING_LENGTH 5120

static struct JsonObject* JsonObject_Null = 0;
static struct JsonObject* JsonObject_True = 0;
static struct JsonObject* JsonObject_False = 0;

JsonObject* jsonobject_new(JsonObjectType type) {
    JsonObject* obj = (JsonObject*)malloc(sizeof(JsonObject));
    memset(obj, 0, sizeof(JsonObject));
    obj->type = type;
    return obj;
}

void jsonparser_init(JsonParser* parser, char* json) {
    parser->content = json;
    parser->length = strlen(json);
    parser->index = 0;
    if(!JsonObject_Null) {
        JsonObject_Null = jsonobject_new(JOT_NULL);
    }
    if(!JsonObject_True) {
        JsonObject_True = jsonobject_new(JOT_BOOL);
        JsonObject_True->vBool = 1;
    }
    if(!JsonObject_False) {
        JsonObject_False = jsonobject_new(JOT_BOOL);
        JsonObject_False->vBool = 0;
    }
}

int jsonparser_iseof(JsonParser* parser) {
    return parser->index >= parser->length;
}

char jsonparser_peek(JsonParser* parser) {
    assert(!jsonparser_iseof(parser));
    return parser->content[parser->index];
}

char jsonparser_next(JsonParser* parser) {
    assert(!jsonparser_iseof(parser));
    return parser->content[parser->index++];
}

void jsonparser_skip_whitespace(JsonParser* parser) {
    while(isspace(jsonparser_peek(parser))) {
        jsonparser_next(parser);
    }
}

void jsonparser_skip_char(JsonParser* parser, char c) {
    assert(jsonparser_next(parser) == c);
}

void jsonparser_syntax_error(JsonParser* parser, const char* msg) {
    printf("syntaxError! invalid char: '%c', msg: %s.\n", jsonparser_peek(parser), msg);
    exit(-1);
}

void jsonparser_parse_symbol(JsonParser* parser, char* symbol, int len) {
    int i = 0;
    while(i < len - 1 && isalpha(jsonparser_peek(parser))) {
        symbol[i++] = jsonparser_next(parser);
    }
    symbol[i] = '\0';
}

JsonObject* jsonparser_parse_other(JsonParser* parser) {
    char symbol[16];
    jsonparser_parse_symbol(parser, symbol, sizeof(symbol));
    if(strcmp(symbol, "null") == 0) {
        return JsonObject_Null;
    } else if(strcmp(symbol, "true") == 0) {
        return JsonObject_True;
    } else if(strcmp(symbol, "false") == 0) {
        return JsonObject_False;
    } else {
        jsonparser_syntax_error(parser, "invalid");
    }
    return 0;
}

JsonObject* jsonparser_parse_number(JsonParser* parser) {
    JsonObject* obj = jsonobject_new(JOT_NUMBER);
    double num = 0;
    int isNeg = 0;
    char s[64];
    char* p = s;
    if(jsonparser_peek(parser) == '-') {
        isNeg = 1;
        *p++ = jsonparser_next(parser);
    }
    while(isdigit(jsonparser_peek(parser))) {
        *p++ = jsonparser_next(parser);
    }
    if(jsonparser_peek(parser) == '.') {
        *p++ = jsonparser_next(parser);
        while(isdigit(jsonparser_peek(parser))) {
            *p++ = jsonparser_next(parser);
        }
    }
    *p = '\0';
    obj->vNumber = atof(s);
    return obj;
}

JsonObject* jsonparser_parse_string(JsonParser* parser) {
    jsonparser_skip_char(parser, '\"');

    JsonObject* obj = jsonobject_new(JOT_STRING);
    int len = 0;
    int oldindex = parser->index;

    // get string length
    while(jsonparser_next(parser) != '\"') {
        len++;
    }
    parser->index = oldindex;

    char* str = (char*)malloc(len + 1);

    for(int i = 0; i < len; i++) {
        str[i] = jsonparser_next(parser);
    }
    str[len] = '\0';

    jsonparser_skip_char(parser, '\"');
    obj->vString = str;
    return obj;
}

char* jsonparser_parse_name(JsonParser* parser) {
    jsonparser_skip_char(parser, '\"');
    int namelen = 0;
    char name[512] = { 0 };
    while(jsonparser_peek(parser) != '\"') {
        name[namelen++] = jsonparser_next(parser);
    }
    name[namelen] = '\0';
    jsonparser_skip_char(parser, '\"');
    return strdup(name);
}

JsonObject* jsonparser_parse_object(JsonParser* parser);
JsonObject* jsonparser_parse_array(JsonParser* parser);

JsonObject* jsonparser_parse_value(JsonParser* parser) {
    char c = jsonparser_peek(parser);
    if(c == '{') {
        return jsonparser_parse_object(parser);
    } else if(c == '[') {
        return jsonparser_parse_array(parser);
    } else if(c == '\"') {
        return jsonparser_parse_string(parser);
    } else if(c == '-' || isdigit(c)) {
        return jsonparser_parse_number(parser);
    } else if(c == 'n' || c == 't' || c == 'f') {
        return jsonparser_parse_other(parser);
    } else {
        jsonparser_syntax_error(parser, "invalid");
    }
    return 0;
}

JsonObject* jsonparser_parse_array(JsonParser* parser) {
    JsonObject* arr = jsonobject_new(JOT_ARRAY);
    JsonObject* cur = jsonobject_new(JOT_NULL);
    arr->vChild = cur;
    jsonparser_skip_char(parser, '[');
    while(jsonparser_peek(parser) != ']') {
        jsonparser_skip_whitespace(parser);
        cur->next = jsonparser_parse_value(parser);
        cur = cur->next;
        jsonparser_skip_whitespace(parser);
        if(jsonparser_peek(parser) == ',') {
            jsonparser_next(parser);
        } else {
            break;   
        }
    }
    jsonparser_skip_char(parser, ']');
    JsonObject* tmp = arr->vChild;
    if(arr->vChild->next != NULL) {
        arr->vChild = arr->vChild->next;
    } else {
        arr->vChild = NULL;
    }
    free(tmp);
    return arr;
}

JsonObject* jsonparser_parse_object(JsonParser* parser) {
    JsonObject* arr = jsonobject_new(JOT_OBJECT);
    JsonObject* cur = jsonobject_new(JOT_NULL);
    arr->vChild = cur;
    jsonparser_skip_char(parser, '{');
    while(jsonparser_peek(parser) != '}') {
        jsonparser_skip_whitespace(parser);
        char* name = jsonparser_parse_name(parser);
        jsonparser_skip_whitespace(parser);
        jsonparser_skip_char(parser, ':');
        jsonparser_skip_whitespace(parser);
        cur->next = jsonparser_parse_value(parser);
        cur->next->name = name;
        cur = cur->next;
        jsonparser_skip_whitespace(parser);
        if(jsonparser_peek(parser) == ',') {
            jsonparser_next(parser);
        } else {
            break;   
        }
    }
    jsonparser_skip_char(parser, '}');
    JsonObject* tmp = arr->vChild;
    if(arr->vChild->next != NULL) {
        arr->vChild = arr->vChild->next;
    } else {
        arr->vChild = NULL;
    }
    free(tmp);
    return arr;
}

JsonObject* jsonparser_parse_content(char* json) {
    JsonParser parser;
    jsonparser_init(&parser, json);
    jsonparser_skip_whitespace(&parser);
    if(jsonparser_peek(&parser) == '{') {
        return jsonparser_parse_object(&parser);
    } else if(jsonparser_peek(&parser) == '[') {
        return jsonparser_parse_array(&parser);
    } else {
        jsonparser_syntax_error(&parser, "invalid");
    }
    return 0;
}

static
char* read_text_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    long length = 0;
    assert(fp);
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* content = (char*)malloc(length + 1);
    fread(content, length, 1, fp);
    content[length] = '\0';
    fclose(fp);
    return content;
}

JsonObject* jsonparser_parse_file(const char* filename) {
    char* json_content = read_text_file(filename);
    JsonObject* root = jsonparser_parse_content(json_content);
    free(json_content);
    return root;
}

int jsonobject_format_name(JsonObject* object, char* str) {
    return sprintf(str, "\"%s\": ", object->name);
}

int jsonobject_format_null(JsonObject* object, char* str) {
    return sprintf(str, "null");
}

int jsonobject_format_bool(JsonObject* object, char* str) {
    if(object->vBool) {
        return sprintf(str, "true");
    } else {
        return sprintf(str, "false");
    }
}

int jsonobject_format_number(JsonObject* object, char* str) {
    if((int)object->vNumber == object->vNumber) {
        return sprintf(str, "%d", (int)(object->vNumber));
    }
    return sprintf(str, "%f", object->vNumber);
}

int jsonobject_format_string(JsonObject* object, char* str) {
    return sprintf(str, "\"%s\"", object->vString);
}

int jsonobject_format_value(JsonObject* object, char* str, int depth);

int jsonobject_format_array(JsonObject* object, char* str, int depth) {
    if(object->vChild == NULL) {
        return sprintf(str, "[]");
    }
    int offset = 0;
    str[offset++] = '[';
    for(JsonObject* cur = object->vChild; cur != NULL; cur = cur->next) {
        str[offset++] = '\n';
        str[offset++] = '\t';
        for(int i = 0; i < depth; i++) {
            str[offset++] = '\t';
        }
        offset += jsonobject_format_value(cur, str + offset, depth + 1);
        if(cur->next != NULL) {
            str[offset++] = ',';
        }
    }
    str[offset++] = '\n';
    for(int i = 0; i < depth; i++) {
        str[offset++] = '\t';
    }
    str[offset++] = ']';
    return offset;
}

int jsonobject_format_object(JsonObject* object, char* str, int depth) {
    if(object->vChild == NULL) {
        return sprintf(str, "{}");
    }
    int offset = 0;
    str[offset++] = '{';
    for(JsonObject* cur = object->vChild; cur != NULL; cur = cur->next) {
        str[offset++] = '\n';
        str[offset++] = '\t';
        for(int i = 0; i < depth; i++) {
            str[offset++] = '\t';
        }
        offset += jsonobject_format_name(cur, str + offset);
        offset += jsonobject_format_value(cur, str + offset, depth + 1);
        if(cur->next != NULL) {
            str[offset++] = ',';
        }
    }
    str[offset++] = '\n';
    for(int i = 0; i < depth; i++) {
        str[offset++] = '\t';
    }
    str[offset++] = '}';
    return offset;
}

int jsonobject_format_value(JsonObject* object, char* str, int depth) {
    assert(object != NULL);
    if(object->type == JOT_NULL) {
        return jsonobject_format_null(object, str);
    } else if(object->type == JOT_BOOL) {
        return jsonobject_format_bool(object, str);
    } else if(object->type == JOT_NUMBER) {
        return jsonobject_format_number(object, str);
    } else if(object->type == JOT_STRING) {
        return jsonobject_format_string(object, str);
    } else if(object->type == JOT_ARRAY) {
        return jsonobject_format_array(object, str, depth);
    } else if(object->type == JOT_OBJECT) {
        return jsonobject_format_object(object, str, depth);
    } else {
        printf("error!\n");
        exit(-1);
    }
    return 0;
}

char* jsonobject_format(JsonObject* object) {
    assert(object != NULL);
    char* str = (char*)malloc(1024 * 1024 * 10); // 10MB
    int len = jsonobject_format_value(object, str, 0);
    str[len] = '\0';
    return str;
}
