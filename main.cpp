#include "rw.h"
#include "jsp.h"
#include "jspbuilder.h"

#include <stdio.h>

enum Platform
{
    PLAT_GC,
    PLAT_PS2,
    PLAT_XBOX
};

static RwBool ReadClump(RpClump* clump, const RwChar* path)
{
    RwStream stream;

    if (!stream.Open(path, rwSTREAMREAD)) {
        return FALSE;
    }

    if (!stream.FindChunk(rwID_CLUMP)) {
        return FALSE;
    }

    if (!clump->StreamRead(&stream)) {
        return FALSE;
    }

    for (RpGeometry& geom : clump->geometries) {
        if (geom.format & rpGEOMETRYNATIVE) {
            printf("Error: Geometry has native data, this is currently unsupported\n");
            return FALSE;
        }
    }

    return TRUE;
}

static RwBool WriteJSP(JSP* jsp, const RwChar* path, Platform platform)
{
    RwStream stream;
    RwBool writeStripVecList;

    if (platform == PLAT_GC) {
        stream.endian = rwBIGENDIAN;
        writeStripVecList = TRUE;
    } else {
        stream.endian = rwLITTLEENDIAN;
        writeStripVecList = FALSE;
    }

    if (!stream.Open(path, rwSTREAMWRITE)) {
        return FALSE;
    }

    if (!jsp->Write(&stream, writeStripVecList)) {
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char** argv)
{
    Platform platform;

    if (argc == 1) {
        printf("Usage: jspgen -p <platform> [input .dff path] [output .jsp path]\n");
        printf("    -p: Platform (gc, ps2, or xbox)\n");
        return 1;
    }

    bool foundPlatform = false;

    int optsEnd = 0;
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if (arg[0] == '-') {
            if (arg[1] == 'p') {
                if (argc < i + 2) {
                    printf("Error: -p must have platform\n");
                    return 1;
                }
                char* plat = argv[i + 1];
                if (strcmp(plat, "gc") == 0) {
                    platform = PLAT_GC;
                } else if (strcmp(plat, "ps2") == 0) {
                    platform = PLAT_PS2;
                } else if (strcmp(plat, "xbox") == 0) {
                    platform = PLAT_XBOX;
                } else {
                    printf("Error: unknown platform %s\n", plat);
                    return 1;
                }
                foundPlatform = true;
                i++;
            } else {
                printf("Error: unknown option %s\n", arg);
                return 1;
            }
            optsEnd = i;
        }
    }

    if (!foundPlatform) {
        printf("Error: platform argument expected\n");
        return 1;
    }

    if (argc - 1 - optsEnd < 2) {
        printf("Error: input and output paths expected\n");
        return 1;
    }

    char* inputPath = argv[optsEnd + 1];
    char* outputPath = argv[optsEnd + 2];

    RpClump clump;
    JSP jsp;

    if (!ReadClump(&clump, inputPath)) {
        return 1;
    }

    JSPBuilder jspBuilder;
    jspBuilder.Build(&jsp, &clump);

    if (!WriteJSP(&jsp, outputPath, platform)) {
        return 1;
    }

    return 0;
}