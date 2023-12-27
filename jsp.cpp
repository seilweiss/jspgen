#include "jsp.h"

#include <assert.h>

#define JSP_RWLIBRARYID 0x1003FFFF // 3.4.0.3

struct ClumpCollBSPHeader
{
    RwUInt32 magic;
    RwUInt32 numBranchNodes;
    RwUInt32 numTriangles;
};

struct JSPHeader
{
    char idtag[4];
    RwUInt32 version;
    RwUInt32 jspNodeCount;
    RwUInt32 clump;
    RwUInt32 colltree;
    RwUInt32 jspNodeList;
};

RwBool ClumpCollBSPTree::Write(RwStream* stream)
{
    assert(stream);

    RwUInt32 headerSize = sizeof(ClumpCollBSPHeader);
    RwUInt32 numBranchNodes = (RwUInt32)branchNodes.size();
    RwUInt32 branchNodesSize = sizeof(ClumpCollBSPBranchNode) * numBranchNodes;
    RwUInt32 numTriangles = (RwUInt32)triangles.size();
    RwUInt32 trianglesSize = sizeof(ClumpCollBSPTriangle) * numTriangles;

    RwChunkHeader chunkHeader;
    chunkHeader.type = 0xBEEF01;
    chunkHeader.length = headerSize + branchNodesSize + trianglesSize;
    chunkHeader.libraryID = JSP_RWLIBRARYID;

    if (!stream->WriteChunkHeader(&chunkHeader)) {
        return FALSE;
    }

    ClumpCollBSPHeader header;
    header.magic = 'LOCC';
    header.numBranchNodes = numBranchNodes;
    header.numTriangles = numTriangles;

    if (stream->Write32(&header, sizeof(header)) != sizeof(header)) {
        return FALSE;
    }

    for (ClumpCollBSPBranchNode& branchNode : branchNodes) {
        stream->Write32(&branchNode.leftInfo);
        stream->Write32(&branchNode.rightInfo);
        stream->Write32(&branchNode.leftValue);
        stream->Write32(&branchNode.rightValue);
    }

    for (ClumpCollBSPTriangle& triangle : triangles) {
        stream->Write16(&triangle.v.i.atomIndex);
        stream->Write16(&triangle.v.i.meshVertIndex);
        stream->Write8(&triangle.flags);
        stream->Write8(&triangle.platData);
        stream->Write16(&triangle.matIndex);
    }

    return TRUE;
}

RwBool JSP::Write(RwStream* stream, RwBool writeStripVecList)
{
    assert(stream);

    if (!colltree.Write(stream)) {
        return FALSE;
    }

    RwUInt32 jspHeaderSize = sizeof(JSPHeader);
    RwUInt32 jspNodeCount = (RwUInt32)jspNodeList.size();
    RwUInt32 jspNodeListSize = sizeof(JSPNodeInfo) * jspNodeCount;

    RwChunkHeader chunkHeader;
    chunkHeader.type = 0xBEEF02;
    chunkHeader.length = jspHeaderSize + jspNodeListSize;
    chunkHeader.libraryID = JSP_RWLIBRARYID;

    if (!stream->WriteChunkHeader(&chunkHeader)) {
        return FALSE;
    }

    JSPHeader header;
    header.idtag[0] = 'J';
    header.idtag[1] = 'S';
    header.idtag[2] = 'P';
    header.idtag[3] = '\0';
    header.version = 3;
    header.jspNodeCount = jspNodeCount;
    header.clump = 0;
    header.colltree = 0;
    header.jspNodeList = 0;

    stream->Write(header.idtag, sizeof(header.idtag));
    stream->Write32(&header.version);
    stream->Write32(&header.jspNodeCount);
    stream->Write32(&header.clump);
    stream->Write32(&header.colltree);
    stream->Write32(&header.jspNodeList);

    if (!jspNodeList.empty()) {
        if (stream->Write32(&jspNodeList[0], jspNodeListSize) != jspNodeListSize) {
            return FALSE;
        }
    }

    if (writeStripVecList) {
        RwUInt32 stripVecCount = (RwUInt32)stripVecList.size();
        RwUInt32 stripVecListSize = sizeof(RwV3d) * stripVecCount;

        chunkHeader.type = 0xBEEF03;
        chunkHeader.length = sizeof(stripVecCount) + stripVecListSize;

        if (!stream->WriteChunkHeader(&chunkHeader)) {
            return FALSE;
        }

        if (stream->Write32(&stripVecCount) != sizeof(stripVecCount)) {
            return FALSE;
        }

        if (!stripVecList.empty()) {
            if (stream->Write32(&stripVecList[0], stripVecListSize) != stripVecListSize) {
                return FALSE;
            }
        }
    }

    return TRUE;
}