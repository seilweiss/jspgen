#pragma once

#include "rw.h"

#include <vector>

enum ClumpCollBSPNodeType
{
    kCLUMPCOLL_TRIANGLE = 1,        // Node is a triangle, index refers to ClumpCollBSPTree::triangles
    kCLUMPCOLL_BRANCH = 2           // Node is a branch, index refers to ClumpCollBSPTree::branchNodes
};

enum ClumpCollBSPTriangleFlags
{
    kCLUMPCOLL_HASNEXT      = 0x1,  // There is another triangle after this one in memory (if not set means this is the last triangle in its chain)
    kCLUMPCOLL_ISREVERSE    = 0x2,  // This triangle is in reverse orientation (every other triangle in a tristrip mesh)
    kCLUMPCOLL_ISSOLID      = 0x4,  // This triangle has collision enabled
    kCLUMPCOLL_ISVISIBLE    = 0x8,  // This triangle is visible in the world
    kCLUMPCOLL_NOSTAND      = 0x10, // Force the player to slide off this triangle instead of stand on it
    kCLUMPCOLL_SHADOW       = 0x20  // Force this triangle to receive shadows in the world
};

#define CLUMPCOLL_MAKEINFO(nodeType, axis, index) (((nodeType) & 0x3) | ((axis) & 0xC) | ((index) << 12))
#define CLUMPCOLL_GETNODETYPE(info) ((info) & 0x3)
#define CLUMPCOLL_GETAXIS(info) ((info) & 0xC)
#define CLUMPCOLL_GETINDEX(info) ((info) >> 12)

struct ClumpCollBSPBranchNode
{
    RwUInt32 leftInfo;
    RwUInt32 rightInfo;
    RwReal leftValue;
    RwReal rightValue;
};

struct ClumpCollBSPVertInfo
{
    RwUInt16 atomIndex;
    RwUInt16 meshVertIndex;
};

struct ClumpCollBSPTriangle
{
    union
    {
        ClumpCollBSPVertInfo i;
        //RwV3d* p;
    } v;
    RwUInt8 flags;
    RwUInt8 platData;
    RwUInt16 matIndex;
};

struct ClumpCollBSPTree
{
    std::vector<ClumpCollBSPBranchNode> branchNodes;
    std::vector<ClumpCollBSPTriangle> triangles;

    RwBool Write(RwStream* stream);
};

enum JSPNodeFlags
{
    JSP_UNK0x1 = 0x1,
    JSP_NOZBUFFER = 0x2,    // Don't write to z-buffer (used for transparent atomics)
    JSP_NOCULL = 0x4        // Don't cull backfaces
};

struct JSPNodeInfo
{
    RwInt32 originalMatIndex;
    RwInt32 nodeFlags;
};

struct JSP
{
    ClumpCollBSPTree colltree;
    std::vector<JSPNodeInfo> jspNodeList;
    std::vector<RwV3d> stripVecList;

    RwBool Write(RwStream* stream, RwBool writeStripVecList);
};