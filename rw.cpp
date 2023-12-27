#include "rw.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/************************************************
* RwBBox
*/

void RwBBox::AddPoint(const RwV3d* v)
{
    if (v->x < inf.x) inf.x = v->x;
    if (v->y < inf.y) inf.y = v->y;
    if (v->z < inf.z) inf.z = v->z;
    if (v->x > sup.x) sup.x = v->x;
    if (v->y > sup.y) sup.y = v->y;
    if (v->z > sup.z) sup.z = v->z;
}

void RwBBox::Calculate(const RwV3d* verts, RwInt32 numVerts)
{
    inf = *verts;
    sup = *verts;

    for (RwInt32 i = 1; i < numVerts; i++) {
        AddPoint(&verts[i]);
    }
}

/************************************************
* RwStream
*/

#define SWAP16(x)                               \
    (  (((x) & 0xFF00) >> 8)                    \
     | (((x) & 0x00FF) << 8) )

#define SWAP32(x)                               \
    (  (((x) & 0xFF000000) >> 24)               \
     | (((x) & 0x00FF0000) >> 8)                \
     | (((x) & 0x0000FF00) << 8)                \
     | (((x) & 0x000000FF) << 24) )

#define SWAP64(x)                               \
    (  (((x) & 0xFF00000000000000) >> 56)       \
     | (((x) & 0x00FF000000000000) >> 40)       \
     | (((x) & 0x0000FF0000000000) >> 24)       \
     | (((x) & 0x000000FF00000000) >> 8)        \
     | (((x) & 0x00000000FF000000) << 8)        \
     | (((x) & 0x0000000000FF0000) << 24)       \
     | (((x) & 0x000000000000FF00) << 40)       \
     | (((x) & 0x00000000000000FF) << 56) )

static void MemSwap16(void* mem, RwUInt32 size)
{
    RwUInt16* p;
    for (p = (RwUInt16*)mem; (char*)p < (char*)mem + size; p++) {
        *p = SWAP16(*p);
    }
}

static void MemSwap32(void* mem, RwUInt32 size)
{
    RwUInt32* p;
    for (p = (RwUInt32*)mem; (char*)p < (char*)mem + size; p++) {
        *p = SWAP32(*p);
    }
}

static void MemSwap64(void* mem, RwUInt32 size)
{
    RwUInt64* p;
    for (p = (RwUInt64*)mem; (char*)p < (char*)mem + size; p++) {
        *p = SWAP64(*p);
    }
}

RwStream::RwStream()
{
    accessType = rwNASTREAMACCESS;
    endian = rwLITTLEENDIAN;
    file = NULL;
}

RwStream::~RwStream()
{
    Close();
}

RwBool RwStream::Open(const RwChar* filename, RwStreamAccessType accessType)
{
    switch (accessType) {
    case rwSTREAMREAD:
        file = fopen(filename, "rb");
        break;
    case rwSTREAMWRITE:
        file = fopen(filename, "wb");
        break;
    default:
        fprintf(stderr, "RwStream error: Invalid stream access type %d\n", accessType);
        return FALSE;
    }

    if (!file) {
        fprintf(stderr, "RwStream error: Failed to open file %s\n", filename);
        return FALSE;
    }

    this->accessType = accessType;

    return TRUE;
}

void RwStream::Close()
{
    if (file) {
        fclose((FILE*)file);
        file = NULL;
    }

    accessType = rwNASTREAMACCESS;
}

RwUInt32 RwStream::Read(void* buffer, RwUInt32 length)
{
    assert(accessType == rwSTREAMREAD);
    assert(file);

    return (RwUInt32)fread(buffer, 1, length, (FILE*)file);
}

RwUInt32 RwStream::Write(const void* buffer, RwUInt32 length)
{
    assert(accessType == rwSTREAMWRITE);
    assert(file);

    return (RwUInt32)fwrite(buffer, 1, length, (FILE*)file);
}

RwBool RwStream::Seek(RwUInt32 pos)
{
    assert(file);

    return fseek((FILE*)file, pos, SEEK_SET) == 0;
}

RwBool RwStream::Skip(RwUInt32 offset)
{
    assert(file);

    return fseek((FILE*)file, offset, SEEK_CUR) == 0;
}

RwUInt32 RwStream::Tell() const
{
    assert(file);

    return (RwUInt32)ftell((FILE*)file);
}

RwUInt32 RwStream::Read8(void* buffer, RwUInt32 length)
{
    return Read(buffer, length);
}

RwUInt32 RwStream::Read16(void* buffer, RwUInt32 length)
{
    return SwapRead(buffer, length, MemSwap16);
}

RwUInt32 RwStream::Read32(void* buffer, RwUInt32 length)
{
    return SwapRead(buffer, length, MemSwap32);
}

RwUInt32 RwStream::Read64(void* buffer, RwUInt32 length)
{
    return SwapRead(buffer, length, MemSwap64);
}

RwUInt32 RwStream::Write8(const void* buffer, RwUInt32 length)
{
    return Write(buffer, length);
}

RwUInt32 RwStream::Write16(const void* buffer, RwUInt32 length)
{
    return SwapWrite(buffer, length, MemSwap16);
}

RwUInt32 RwStream::Write32(const void* buffer, RwUInt32 length)
{
    return SwapWrite(buffer, length, MemSwap32);
}

RwUInt32 RwStream::Write64(const void* buffer, RwUInt32 length)
{
    return SwapWrite(buffer, length, MemSwap64);
}

RwUInt32 RwStream::SwapRead(void* buffer, RwUInt32 length, void(*swap)(void*, RwUInt32))
{
    assert(accessType == rwSTREAMREAD);
    assert(file);
    assert(buffer);
    assert(length);
    assert(swap);

    if (endian == rwENDIAN) {
        return Read(buffer, length);
    }

    RwUInt32 bytesRead = Read(buffer, length);

    swap(buffer, length);

    return bytesRead;
}

RwUInt32 RwStream::SwapWrite(const void* buffer, RwUInt32 length, void(*swap)(void*, RwUInt32))
{
    assert(accessType == rwSTREAMWRITE);
    assert(file);
    assert(buffer);
    assert(length);
    assert(swap);

    if (endian == rwENDIAN) {
        return Write(buffer, length);
    }

    RwUInt8 convertBuffer[256];
    RwUInt32 totalBytesWritten = 0;
    RwUInt32 bytesLeft = length;

    while (bytesLeft) {
        RwUInt32 bytesToWrite = bytesLeft < sizeof(convertBuffer) ? bytesLeft : sizeof(convertBuffer);

        memcpy(convertBuffer, buffer, bytesToWrite);
        swap(convertBuffer, bytesToWrite);

        RwUInt32 bytesWritten = Write(convertBuffer, bytesToWrite);

        totalBytesWritten += bytesWritten;

        if (bytesWritten != bytesToWrite) {
            break;
        }

        bytesLeft -= bytesWritten;
        buffer = (RwUInt8*)buffer + bytesWritten;
    }

    return totalBytesWritten;
}

RwBool RwStream::ReadChunkHeader(RwChunkHeader* header)
{
    assert(header);

    RwEndian oldEndian = endian;

    endian = rwLITTLEENDIAN;
    RwUInt32 size = Read32(header, sizeof(RwChunkHeader));
    endian = oldEndian;

    return size == sizeof(RwChunkHeader);
}

RwBool RwStream::WriteChunkHeader(RwChunkHeader* header)
{
    assert(header);

    RwEndian oldEndian = endian;

    endian = rwLITTLEENDIAN;
    RwUInt32 size = Write32(header, sizeof(RwChunkHeader));
    endian = oldEndian;

    return size == sizeof(RwChunkHeader);
}

RwBool RwStream::FindChunk(RwUInt32 type, RwUInt32* lengthOut)
{
    RwChunkHeader header;

    while (ReadChunkHeader(&header)) {
        if (header.type == type) {
            if (lengthOut) {
                *lengthOut = header.length;
            }

            return TRUE;
        }

        if (!Skip(header.length)) {
            printf("RwStream error: Failed to skip chunk %d (length %d) while looking for chunk %d\n",
                   header.type, header.length, type);
            return FALSE;
        }
    }

    return FALSE;
}

/************************************************
* RpMeshHeader
*/

struct BinMeshHeader
{
    RwUInt32 flags;
    RwUInt32 numMeshes;
    RwUInt32 totalIndicesInMesh;
};

struct BinMesh
{
    RwUInt32 numIndices;
    RwInt32 matIndex;
};

RwBool RpMeshHeader::StreamRead(RwStream* stream, RpGeometry* geometry)
{
    assert(stream);
    assert(geometry);

    BinMeshHeader mh;
    if (stream->Read32(&mh, sizeof(mh)) != sizeof(mh)) {
        return FALSE;
    }

    flags = mh.flags;
    totalIndicesInMesh = mh.totalIndicesInMesh;
    meshes.resize(mh.numMeshes);

    for (RwUInt32 i = 0; i < mh.numMeshes; i++) {
        BinMesh m;
        if (stream->Read32(&m, sizeof(m)) != sizeof(m)) {
            return FALSE;
        }

        meshes[i].matIndex = m.matIndex;

        if (!(geometry->format & rpGEOMETRYNATIVE)) {
            meshes[i].indices.resize(m.numIndices);

            RwUInt32 indexBuffer[256];
            RxVertexIndex* dest = &meshes[i].indices[0];
            RwUInt32 remainingIndices = m.numIndices;
            while (remainingIndices) {
                RwUInt32 readIndices = (remainingIndices < 256) ? remainingIndices : 256;
                RwUInt32 readSize = readIndices * sizeof(RwUInt32);

                if (stream->Read32(indexBuffer, readSize) != readSize) {
                    return FALSE;
                }

                for (RwUInt32 j = 0; j < readIndices; j++) {
                    *dest++ = (RxVertexIndex)indexBuffer[j];
                }

                remainingIndices -= readIndices;
            }
        }
    }

    return TRUE;
}

/************************************************
* RpGeometry
*/

struct BinGeometry
{
    RwInt32 format;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numMorphTargets;
};

struct BinTriangle
{
    RwUInt32 vertex01;
    RwUInt32 vertex2Mat;
};

struct BinMorphTarget
{
    RwSphere boundingSphere;
    RwBool pointsPresent;
    RwBool normalsPresent;
};

RwBool RpGeometry::StreamRead(RwStream* stream)
{
    assert(stream);

    if (!stream->FindChunk(rwID_STRUCT)) {
        return FALSE;
    }

    BinGeometry g;
    if (stream->Read32(&g, sizeof(g)) != sizeof(g)) {
        return FALSE;
    }

    format = g.format;
    numVertices = g.numVertices;

    if (g.format & 0xFF0000) {
        numTexCoordSets = (g.format & 0xFF0000) >> 16;
    } else if (g.format & rpGEOMETRYTEXTURED2) {
        numTexCoordSets = 2;
    } else if (g.format & rpGEOMETRYTEXTURED) {
        numTexCoordSets = 1;
    } else {
        numTexCoordSets = 0;
    }

    if (!(g.format & rpGEOMETRYNATIVE)) {
        if (g.numVertices) {
            if (g.format & rpGEOMETRYPRELIT) {
                RwUInt32 size = g.numVertices * sizeof(RwRGBA);

                preLitLum.resize(g.numVertices);
                if (stream->Read(&preLitLum[0], size) != size) {
                    return FALSE;
                }
            }

            if (numTexCoordSets > 0) {
                RwUInt32 size = g.numVertices * sizeof(RwTexCoords);

                for (RwInt32 i = 0; i < numTexCoordSets; i++) {
                    texCoords[i].resize(g.numVertices);
                    if (stream->Read32(&texCoords[i][0], size) != size) {
                        return FALSE;
                    }
                }
            }

            if (g.numTriangles) {
                RwUInt32 size = g.numTriangles * sizeof(BinTriangle);

                triangles.resize(g.numTriangles);
                if (stream->Read32(&triangles[0], size) != size) {
                    return FALSE;
                }

                for (RwInt32 i = 0; i < g.numTriangles; i++) {
                    BinTriangle* src = (BinTriangle*)&triangles[i];
                    RpTriangle* dst = &triangles[i];

                    dst->vertIndex[0] = (src->vertex01 >> 16) & 0xFFFF;
                    dst->vertIndex[1] = src->vertex01 & 0xFFFF;
                    dst->vertIndex[2] = (src->vertex2Mat >> 16) & 0xFFFF;
                    dst->matIndex = src->vertex2Mat & 0xFFFF;
                }
            }
        }
    }

    if (g.numMorphTargets) {
        morphTargets.resize(g.numMorphTargets);

        for (RwInt32 i = 0; i < g.numMorphTargets; i++) {
            BinMorphTarget mt;
            if (stream->Read32(&mt, sizeof(mt)) != sizeof(mt)) {
                return FALSE;
            }

            morphTargets[i].boundingSphere = mt.boundingSphere;

            if (mt.pointsPresent) {
                RwUInt32 size = g.numVertices * sizeof(RwV3d);

                morphTargets[i].verts.resize(g.numVertices);
                if (stream->Read32(&morphTargets[i].verts[0], size) != size) {
                    return FALSE;
                }
            }

            if (mt.normalsPresent) {
                RwUInt32 size = g.numVertices * sizeof(RwV3d);

                morphTargets[i].normals.resize(g.numVertices);
                if (stream->Read32(&morphTargets[i].normals[0], size) != size) {
                    return FALSE;
                }
            }
        }
    }

    // TODO read material list

    if (stream->FindChunk(rwID_EXTENSION)) {
        if (stream->FindChunk(rwID_BINMESHPLUGIN)) {
            if (!mesh.StreamRead(stream, this)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/************************************************
* RpClump
*/

struct BinClump
{
    RwInt32 numAtomics;
    RwInt32 numLights;
    RwInt32 numCameras;
};

struct BinFrame
{
    RwV3d right, up, at, pos;
    RwInt32 parentIndex;
    RwUInt32 data;
};

struct BinAtomic
{
    RwInt32 frameIndex;
    RwInt32 geomIndex;
    RwInt32 flags;
    RwInt32 unused;
};

RwBool RpClump::StreamRead(RwStream* stream)
{
    assert(stream);

    if (!stream->FindChunk(rwID_STRUCT)) {
        return FALSE;
    }

    BinClump c;

    if (stream->Read32(&c, sizeof(c)) != sizeof(c)) {
        return FALSE;
    }

    atomics.reserve(c.numAtomics);

    if (!stream->FindChunk(rwID_FRAMELIST)) {
        return FALSE;
    }

    if (!ReadFrameList(stream)) {
        return FALSE;
    }

    if (!stream->FindChunk(rwID_GEOMETRYLIST)) {
        return FALSE;
    }

    if (!ReadGeometryList(stream)) {
        return FALSE;
    }

    for (RwInt32 i = 0; i < c.numAtomics; i++) {
        if (!stream->FindChunk(rwID_ATOMIC)) {
            return FALSE;
        }

        if (!ReadAtomic(stream)) {
            return FALSE;
        }
    }

    return TRUE;
}

RwBool RpClump::ReadFrameList(RwStream* stream)
{
    assert(stream);

    if (!stream->FindChunk(rwID_STRUCT)) {
        return FALSE;
    }

    RwInt32 numFrames;
    if (stream->Read32(&numFrames, sizeof(numFrames)) != sizeof(numFrames)) {
        return FALSE;
    }

    frames.resize(numFrames);

    for (RwInt32 i = 0; i < numFrames; i++) {
        BinFrame f;
        if (stream->Read32(&f, sizeof(f)) != sizeof(f)) {
            return FALSE;
        }

        frames[i].matrix.right = f.right;
        frames[i].matrix.up = f.up;
        frames[i].matrix.at = f.at;
        frames[i].matrix.pos = f.pos;
        frames[i].parent = (f.parentIndex >= 0) ? &frames[f.parentIndex] : NULL;
    }

    return TRUE;
}

RwBool RpClump::ReadGeometryList(RwStream* stream)
{
    assert(stream);

    if (!stream->FindChunk(rwID_STRUCT)) {
        return FALSE;
    }

    RwInt32 numGeoms;
    if (stream->Read32(&numGeoms, sizeof(numGeoms)) != sizeof(numGeoms)) {
        return FALSE;
    }

    geometries.resize(numGeoms);

    for (RwInt32 i = 0; i < numGeoms; i++) {
        if (!stream->FindChunk(rwID_GEOMETRY)) {
            return FALSE;
        }

        if (!geometries[i].StreamRead(stream)) {
            return FALSE;
        }
    }

    return TRUE;
}

RwBool RpClump::ReadAtomic(RwStream* stream)
{
    assert(stream);

    if (!stream->FindChunk(rwID_STRUCT)) {
        return FALSE;
    }

    BinAtomic a;
    if (stream->Read32(&a, sizeof(a)) != sizeof(a)) {
        return FALSE;
    }

    RpAtomic atom;
    atom.flags = a.flags;
    atom.frame = &frames[a.frameIndex];
    atom.geometry = &geometries[a.geomIndex];

    atomics.push_back(atom);

    return TRUE;
}