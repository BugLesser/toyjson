#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "toyjson.h"

int main(int argc, char const *argv[]) {
    if(argc == 1) {
        printf("usage: %s file.json", argv[0]);
        return -1;
    }
    JsonObject* root = jsonparser_parse_file(argv[1]);
    printf("%s\n", jsonobject_format(root));
    return 0;
}
