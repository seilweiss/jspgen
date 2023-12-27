#pragma once

#include <stdint.h>
#include <vector>

typedef int8_t RwInt8;
typedef int16_t RwInt16;
typedef int32_t RwInt32;
typedef int64_t RwInt64;
typedef uint8_t RwUInt8;
typedef uint16_t RwUInt16;
typedef uint32_t RwUInt32;
typedef uint64_t RwUInt64;
typedef RwInt32 RwBool;
typedef float RwReal;
typedef char RwChar;

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

enum RwEndian
{
    rwLITTLEENDIAN,
    rwBIGENDIAN
};

#define rwENDIAN rwLITTLEENDIAN

struct RwChunkHeader
{
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 libraryID;
};

enum RwStreamAccessType
{
    rwNASTREAMACCESS,
    rwSTREAMREAD,
    rwSTREAMWRITE
};

struct RwStream
{
    RwStreamAccessType accessType;
    RwEndian endian;
    void* file;

    RwStream();
    ~RwStream();

    RwBool Open(const RwChar* filename, RwStreamAccessType accessType);
    void Close();
    RwUInt32 Read(void* buffer, RwUInt32 length);
    RwUInt32 Write(const void* buffer, RwUInt32 length);
    RwBool Seek(RwUInt32 pos);
    RwBool Skip(RwUInt32 offset);
    RwUInt32 Tell() const;

    RwUInt32 Read8(void* buffer, RwUInt32 length = sizeof(RwUInt8));
    RwUInt32 Read16(void* buffer, RwUInt32 length = sizeof(RwUInt16));
    RwUInt32 Read32(void* buffer, RwUInt32 length = sizeof(RwUInt32));
    RwUInt32 Read64(void* buffer, RwUInt32 length = sizeof(RwUInt64));
    RwUInt32 Write8(const void* buffer, RwUInt32 length = sizeof(RwUInt8));
    RwUInt32 Write16(const void* buffer, RwUInt32 length = sizeof(RwUInt16));
    RwUInt32 Write32(const void* buffer, RwUInt32 length = sizeof(RwUInt32));
    RwUInt32 Write64(const void* buffer, RwUInt32 length = sizeof(RwUInt64));

    RwBool ReadChunkHeader(RwChunkHeader* header);
    RwBool WriteChunkHeader(RwChunkHeader* header);

    RwBool FindChunk(RwUInt32 type, RwUInt32* lengthOut = NULL);

private:
    RwUInt32 SwapRead(void* buffer, RwUInt32 length, void(*swap)(void*, RwUInt32));
    RwUInt32 SwapWrite(const void* buffer, RwUInt32 length, void(*swap)(void*, RwUInt32));
};

enum RwPluginID
{
    rwID_NAOBJECT = 0x00,
    rwID_STRUCT = 0x01,
    rwID_EXTENSION = 0x03,
    rwID_FRAMELIST = 0x0E,
    rwID_GEOMETRY = 0x0F,
    rwID_CLUMP = 0x10,
    rwID_ATOMIC = 0x14,
    rwID_GEOMETRYLIST = 0x1A,
    rwID_BINMESHPLUGIN = 0x50E,
};

struct RwV3d
{
    RwReal x, y, z;
};

struct RwMatrix
{
    RwV3d right;
    RwUInt32 pad0;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    RwV3d pos;
    RwUInt32 pad3;
};

struct RwRGBA
{
    RwUInt8 red, green, blue, alpha;
};

struct RwTexCoords
{
    RwReal u, v;
};

struct RwSphere
{
    RwV3d center;
    RwReal radius;
};

struct RwBBox
{
    RwV3d sup, inf;

    void AddPoint(const RwV3d* vertex);
    void Calculate(const RwV3d* verts, RwInt32 numVerts);
};

enum RwPlaneType
{
    rwXPLANE = 0,
    rwYPLANE = 4,
    rwZPLANE = 8
};

#define GETCOORD(vect, y) (*(RwReal*)(((RwUInt8*)(&((vect).x)))+(RwInt32)(y)))
#define SETCOORD(vect, y, value) (((*(RwReal*)(((RwUInt8*)(&((vect).x)))+(RwInt32)(y))))=(value))

struct RwFrame
{
    RwFrame* parent;
    RwMatrix matrix;
};

struct RpGeometry;

struct RpTriangle
{
    RwUInt16 vertIndex[3];
    RwUInt16 matIndex;
};

struct RpMorphTarget
{
    RwSphere boundingSphere;
    std::vector<RwV3d> verts;
    std::vector<RwV3d> normals;
};

typedef RwUInt16 RxVertexIndex;

struct RpMesh
{
    RwInt32 matIndex;
    std::vector<RxVertexIndex> indices;
};

struct RpMeshHeader
{
    RwUInt32 flags;
    RwUInt32 totalIndicesInMesh;
    std::vector<RpMesh> meshes;

    RwBool StreamRead(RwStream* stream, RpGeometry* geometry);
};

enum RpGeometryFlag
{
    rpGEOMETRYTRISTRIP = 0x00000001,
    rpGEOMETRYPOSITIONS = 0x00000002,
    rpGEOMETRYTEXTURED = 0x00000004,
    rpGEOMETRYPRELIT = 0x00000008,
    rpGEOMETRYNORMALS = 0x00000010,
    rpGEOMETRYLIGHT = 0x00000020,
    rpGEOMETRYMODULATEMATERIALCOLOR = 0x00000040,
    rpGEOMETRYTEXTURED2 = 0x00000080,
    rpGEOMETRYNATIVE = 0x01000000,
    rpGEOMETRYNATIVEINSTANCE = 0x02000000,
    rpGEOMETRYFLAGSMASK = 0x000000FF,
    rpGEOMETRYNATIVEFLAGSMASK = 0x0F000000
};

#define rwMAXTEXTURECOORDS 8

struct RpGeometry
{
    RwInt32 format;
    RwInt32 numVertices;
    RwInt32 numTexCoordSets;
    RpMeshHeader mesh;
    std::vector<RwRGBA> preLitLum;
    std::vector<RwTexCoords> texCoords[rwMAXTEXTURECOORDS];
    std::vector<RpTriangle> triangles;
    std::vector<RpMorphTarget> morphTargets;

    RwBool StreamRead(RwStream* stream);
};

enum RpAtomicFlag
{
    rpATOMICCOLLISIONTEST = 0x01,
    rpATOMICRENDER = 0x04
};

struct RpAtomic
{
    RwInt32 flags;
    RwFrame* frame;
    RpGeometry* geometry;
};

struct RpClump
{
    std::vector<RwFrame> frames;
    std::vector<RpGeometry> geometries;
    std::vector<RpAtomic> atomics;

    RwBool StreamRead(RwStream* stream);

private:
    RwBool ReadFrameList(RwStream* stream);
    RwBool ReadGeometryList(RwStream* stream);
    RwBool ReadAtomic(RwStream* stream);
};