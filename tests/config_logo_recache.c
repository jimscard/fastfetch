#include "options/logo.h"
#include "common/textModifier.h"
#include "3rdparty/yyjson/yyjson.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char* msg, int line) {
    fprintf(stderr, FASTFETCH_TEXT_MODIFIER_ERROR "[%d] %s\n" FASTFETCH_TEXT_MODIFIER_RESET, line, msg);
    exit(1);
}

#define VERIFY(expr) do { if (!(expr)) fail(#expr, __LINE__); } while(0)

static void parse_and_assert(const char* json, FFLogoCacheStrategy expected) {
    FFOptionsLogo options;
    ffOptionsInitLogo(&options);

    yyjson_doc* doc = yyjson_read_opts((char*)json, strlen(json), YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS, NULL, NULL);
    if (!doc) fail("yyjson_read_opts failed", __LINE__);
    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* problemKey = NULL;
    const char* err = ffOptionsParseLogoJsonConfig(&options, root, &problemKey);
    if (err) {
        fprintf(stderr, FASTFETCH_TEXT_MODIFIER_ERROR "parse error: %s\n" FASTFETCH_TEXT_MODIFIER_RESET, err);
        yyjson_doc_free(doc);
        exit(1);
    }

    VERIFY(options.cache == expected);

    yyjson_doc_free(doc);
}

int main(void) {
    // logo.recache presence => treated as regen (backwards-compatible alias)
    parse_and_assert("{ \"logo\": { \"recache\": true } }", FF_LOGO_CACHE_REGEN);
    parse_and_assert("{ \"logo\": { \"recache\": false } }", FF_LOGO_CACHE_REGEN);
    parse_and_assert("{ \"logo\": { \"recache\": \"regen\" } }", FF_LOGO_CACHE_REGEN);

    // logo.cache variations
    parse_and_assert("{ \"logo\": { \"cache\": true } }", FF_LOGO_CACHE_ON);
    parse_and_assert("{ \"logo\": { \"cache\": false } }", FF_LOGO_CACHE_OFF);
    parse_and_assert("{ \"logo\": { \"cache\": \"regen\" } }", FF_LOGO_CACHE_REGEN);

    puts("\033[32mAll tests passed!" FASTFETCH_TEXT_MODIFIER_RESET);
    return 0;
}
