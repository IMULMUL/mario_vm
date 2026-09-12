/* Temporary compile-only probe (not part of the build): js_compile a file
 * without running it, to separate compiler hangs from runtime hangs. */
#include <stdio.h>
#include <stdlib.h>
#include "mario.h"

extern bool js_compile(bytecode_t *bc, const char* input);

static char* read_file(const char* p, long* n) {
    FILE* f = fopen(p, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END); *n = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = (char*)malloc(*n + 1);
    if(fread(b, 1, *n, f) != (size_t)*n) { fclose(f); free(b); return NULL; }
    b[*n] = 0; fclose(f);
    return b;
}

static void probe_out(const char* s) { fputs(s, stdout); }
static void* probe_malloc(uint32_t sz) { return malloc(sz); }

int main(int argc, char** argv) {
    if(argc < 2) return 2;
    _platform_malloc = probe_malloc;
    _platform_free = free;
    _platform_out = probe_out;
    long n = 0;
    char* src = read_file(argv[1], &n);
    if(!src) { printf("read fail\n"); return 2; }
    bytecode_t bc;
    bc_init(&bc);
    bool ok = js_compile(&bc, src);
    printf("compile %s (cindex=%d)\n", ok ? "OK" : "FAIL", (int)bc.cindex);
    return ok ? 0 : 1;
}
