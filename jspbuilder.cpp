#include "jspbuilder.h"

#include <stdio.h>
#include <assert.h>

#define MAXBSPDEPTH 32
#define MAXTRIANGLES 5

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf
#endif

void JSPBuilder::Build(JSP* jsp, RpClump* clump)
{
    assert(jsp);
    assert(clump);

    mJSP = jsp;
    mClump = clump;
    mBspDepth = 0;
    mStats.maxDepthReached = 0;
    mTriangles.clear();

    BuildJSPNodeList();
    BuildStripVecList();
    BuildBSPTree();

    printf("Branch nodes: %d\n", (RwUInt32)mJSP->colltree.branchNodes.size());
    printf("Triangles: %d\n", (RwUInt32)mJSP->colltree.triangles.size());
    printf("Max BSP depth reached: %d\n", mStats.maxDepthReached);
}

void JSPBuilder::BuildJSPNodeList()
{
    mJSP->jspNodeList.reserve(mClump->atomics.size());

    // Nodes are stored in reverse order
    for (auto it = mClump->atomics.rbegin(); it != mClump->atomics.rend(); it++) {
        RpAtomic& atom = *it;

        // TODO add support for z-buffer and culling modes, somehow
        JSPNodeInfo nodeInfo;
        nodeInfo.originalMatIndex = -1; // This value doesn't seem to matter?
        nodeInfo.nodeFlags = JSP_UNK0x1;

        mJSP->jspNodeList.push_back(nodeInfo);
    }
}

void JSPBuilder::BuildStripVecList()
{
    // Generate a cache of every triangle's vertices.
    // This gets written to the file for the GameCube version.
    // This speeds up loading at the cost of increased file size.
    // I believe on other platforms this list gets generated at runtime.

    RwUInt16 totalIndices = 0;
    for (RpAtomic& atom : mClump->atomics) {
        totalIndices += atom.geometry->mesh.totalIndicesInMesh;
    }
    mJSP->stripVecList.reserve(totalIndices);

    // Need to loop through atomics in reverse
    for (auto it = mClump->atomics.rbegin(); it != mClump->atomics.rend(); it++) {
        RpAtomic& atom = *it;
        RpMorphTarget& mt = atom.geometry->morphTargets[0];
        for (RpMesh& mesh : atom.geometry->mesh.meshes) {
            for (RxVertexIndex idx : mesh.indices) {
                mJSP->stripVecList.push_back(mt.verts[idx]);
            }
        }
    }
}

void JSPBuilder::BuildBSPTree()
{
    // Load all the triangles from the model, unsorted.
    InitTriangles();

    // Create a bbox surrounding the whole model.
    RwBBox bbox;
    InitBBox(&bbox);

    // Make the tree 4Head
    RecurseTriangles(0, mTriangles.size() - 1, &bbox);

    // Now all our triangles are neatly sorted, copy them into the BSP tree.
    CopyTriangles();
}

// Initialize a bbox that surrounds the entire model
void JSPBuilder::InitBBox(RwBBox* bbox)
{
    bbox->inf.x = bbox->inf.y = bbox->inf.z = INFINITY;
    bbox->sup.x = bbox->sup.y = bbox->sup.z = -INFINITY;

#if 0
    for (RpAtomic& atom : mClump->atomics) {
        for (RwV3d& v : atom.geometry->morphTargets[0].verts) {
            bbox->AddPoint(&v);
        }
    }
#else
    // Loop through all the geometries. Should be faster than looping through atomics (especially if they share geometries).
    // If there's unused geometries, though, the bbox may start bigger than intended, which will lead to a less optimal tree.
    // Correctly exported dffs shouldn't have that issue though.
    for (RpGeometry& geom : mClump->geometries) {
        for (RwV3d& v : geom.morphTargets[0].verts) {
            bbox->AddPoint(&v);
        }
    }
#endif
}

static RwBool IsDegenerateTriangle(RwUInt16* indices)
{
    return indices[0] == indices[1] ||
           indices[0] == indices[2] ||
           indices[1] == indices[2];
}

void JSPBuilder::InitTriangles()
{
    // Every triangle starts out in one big chain.
    // They are also all solid for now.
    // TODO need a way to support nonsolid atomics/triangles
    TriangleData tri;
    tri.bspTri.flags = kCLUMPCOLL_HASNEXT | kCLUMPCOLL_ISSOLID;
    tri.bspTri.platData = 0; // TODO see what this means on PS2/Xbox. It's unused on GameCube

    RwUInt16 stripVecOffset = 0;

    for (RwUInt16 atomIndex = (RwUInt16)mClump->atomics.size(); atomIndex--;) {
        RpAtomic& atom = mClump->atomics[atomIndex];
        RpMorphTarget& mt = atom.geometry->morphTargets[0];
        RwUInt16 meshVertOffset = 0;

        tri.bspTri.v.i.atomIndex = atomIndex;

        // Triangles are marked as visible if their containing atomic is visible.
        // I believe this is only used for shadow rendering.
        if (atom.flags & rpATOMICRENDER) {
            tri.bspTri.flags |= kCLUMPCOLL_ISVISIBLE;
        } else {
            tri.bspTri.flags &= ~kCLUMPCOLL_ISVISIBLE;
        }

        // TODO need to validate mesh is tristrip (atom.geometry->mesh.flags contains primitive type)
        // Non-tristrip is not supported

        for (RwUInt16 meshIndex = 0; meshIndex < (RwUInt16)atom.geometry->mesh.meshes.size(); meshIndex++) {
            RpMesh& mesh = atom.geometry->mesh.meshes[meshIndex];

            tri.bspTri.matIndex = mesh.matIndex;

            for (RwUInt16 vertIndex = 0; vertIndex < (RwUInt16)mesh.indices.size() - 2; vertIndex++) {
                // Filter out degenerate triangles (triangles with zero area)
                if (IsDegenerateTriangle(&mesh.indices[vertIndex])) {
                    continue;
                }

                tri.bspTri.v.i.meshVertIndex = meshVertOffset + vertIndex;
                tri.p = &mJSP->stripVecList[stripVecOffset + vertIndex];

                // Calculate the minimum and maximum coords of each triangle.
                // These are used to speedup partitioning
                for (RwUInt32 axis = 0; axis < sizeof(RwV3d); axis += 4) {
                    RwReal v0 = GETCOORD(tri.p[0], axis);
                    RwReal v1 = GETCOORD(tri.p[1], axis);
                    RwReal v2 = GETCOORD(tri.p[2], axis);

                    RwReal min = v0;
                    RwReal max = v0;
                    if (v1 < min) min = v1;
                    if (v2 < min) min = v2;
                    if (v1 > max) max = v1;
                    if (v2 > max) max = v2;

                    SETCOORD(tri.min, axis, min);
                    SETCOORD(tri.max, axis, max);
                }
                
                // Since this is a tristrip, every 2nd triangle is in reverse orientation (clockwise).
                // This will be accounted for during collision checking at runtime.
                if (vertIndex % 2) {
                    tri.bspTri.flags |= kCLUMPCOLL_ISREVERSE;
                } else {
                    tri.bspTri.flags &= ~kCLUMPCOLL_ISREVERSE;
                }

                mTriangles.push_back(tri);
            }

            stripVecOffset += (RwUInt16)mesh.indices.size();
            meshVertOffset += (RwUInt16)mesh.indices.size();
        }
    }
}

// Hoare partition scheme implementation.
// This sorts a span of triangles into left and right regions, in-place.
// It returns the index of the last triangle in the left region.
// https://en.wikipedia.org/wiki/Quicksort#Hoare_partition_scheme
RwInt32 JSPBuilder::PartitionTriangles(RwInt32 lo, RwInt32 hi, RwReal splitPlane, RwPlaneType axis)
{
    RwInt32 i = lo - 1;
    RwInt32 j = hi + 1;

    while (true) {
        // We check the center of the triangle against the split coordinate.
        // If it's less than the split coordinate (if the triangle is mostly on the left side), it goes in the left region.
        // If it's greater or equal than the split coordinate (if the triangle is mostly on the right side), it goes in the right region.

        do { i++; } while (i <= hi && mTriangles[i].GetCenter(axis) < splitPlane);
        do { j--; } while (j >= lo && mTriangles[j].GetCenter(axis) >= splitPlane);

        if (i >= j) return j;

        TriangleData tmp = mTriangles[i];
        mTriangles[i] = mTriangles[j];
        mTriangles[j] = tmp;
    }
}

// Here we choose where to split the bounding box and along what axis.
// There are many different ways of choosing this, some of which lead to more optimal trees than others.
void JSPBuilder::ChooseSplitPlane(RwBBox* bbox, RwInt32 lo, RwInt32 hi, RwReal* splitPlaneOut, RwPlaneType* axisOut)
{
    // Currently, we choose the longest side of the bbox and split it down the middle.

    RwV3d dim;
    dim.x = bbox->sup.x - bbox->inf.x;
    dim.y = bbox->sup.y - bbox->inf.y;
    dim.z = bbox->sup.z - bbox->inf.z;

    RwPlaneType axis = rwXPLANE;
    if (dim.y > GETCOORD(dim, axis)) axis = rwYPLANE;
    if (dim.z > GETCOORD(dim, axis)) axis = rwZPLANE;

    RwReal splitPlane = (GETCOORD(bbox->inf, axis) + GETCOORD(bbox->sup, axis)) / 2.0f;

    *splitPlaneOut = splitPlane;
    *axisOut = axis;
}

// Here we recursively partition and sort the triangles in-place, using a quicksort-like algorithm.
// We also create the branch nodes in the process.
void JSPBuilder::RecurseTriangles(RwInt32 lo, RwInt32 hi, RwBBox* bbox)
{
    assert(lo < hi);

    dprintf("BSP Depth: %d\n", mBspDepth);

    if (mBspDepth > mStats.maxDepthReached) {
        mStats.maxDepthReached = mBspDepth;
    }

    // Here we choose a (hopefully) good split plane.
    // This affects how balanced the tree is.
    RwReal splitPlane;
    RwPlaneType axis;
    ChooseSplitPlane(bbox, lo, hi, &splitPlane, &axis);

    dprintf("(%f %f %f) (%f %f %f)\n",
           bbox->inf.x, bbox->inf.y, bbox->inf.z,
           bbox->sup.x, bbox->sup.y, bbox->sup.z);

    dprintf("Split plane: %f, axis: ", splitPlane);
    switch (axis) {
    case rwXPLANE: dprintf("X\n"); break;
    case rwYPLANE: dprintf("Y\n"); break;
    case rwZPLANE: dprintf("Z\n"); break;
    }

    // Here we partition the triangles along the split plane, sorting them into left and right regions.
    RwInt32 p = PartitionTriangles(lo, hi, splitPlane, axis);

    // Calculate left and right overlap planes.
    // Left plane is the maximum coordinate of the left triangles.
    // Right plane is the minimum coordinate of the right triangles.
    RwReal leftPlane = -INFINITY;
    RwReal rightPlane = INFINITY;

    for (RwInt32 i = lo; i <= p; i++) {
        assert(mTriangles[i].GetCenter(axis) < splitPlane);
        RwReal max = GETCOORD(mTriangles[i].max, axis);
        if (max > leftPlane) leftPlane = max;
    }

    for (RwInt32 i = p + 1; i <= hi; i++) {
        assert(mTriangles[i].GetCenter(axis) >= splitPlane);
        RwReal min = GETCOORD(mTriangles[i].min, axis);
        if (min < rightPlane) rightPlane = min;
    }

    RwUInt32 numLeft = p + 1 - lo;
    RwUInt32 numRight = hi - p;
    RwBool doneLeft = FALSE;
    RwBool doneRight = FALSE;

    // We can stop branching once we only have a few triangles left, or if we've hit the BSP depth limit.
    if (numLeft <= MAXTRIANGLES || mBspDepth >= MAXBSPDEPTH - 1) {
        doneLeft = TRUE;
    }

    if (numRight <= MAXTRIANGLES || mBspDepth >= MAXBSPDEPTH - 1) {
        doneRight = TRUE;
    }

    dprintf("Left %d, Right %d\n", numLeft, numRight);
    dprintf("Left %f, Right %f\n", leftPlane, rightPlane);

    // Here we create the branch node for the current level and add it to the tree.
    RwUInt32 nodeIndex = (RwUInt32)mJSP->colltree.branchNodes.size();

    mJSP->colltree.branchNodes.emplace_back();

    mJSP->colltree.branchNodes[nodeIndex].leftValue = leftPlane;
    mJSP->colltree.branchNodes[nodeIndex].rightValue = rightPlane;

    if (!doneLeft) {
        // Store a pointer to the left branch node.
        mJSP->colltree.branchNodes[nodeIndex].leftInfo = CLUMPCOLL_MAKEINFO(kCLUMPCOLL_BRANCH, axis, mJSP->colltree.branchNodes.size());

        // Shrink the bbox to the left region.
        RwBBox leftBBox = *bbox;
        SETCOORD(leftBBox.sup, axis, leftPlane);

        // Recurse down the left branch.
        mBspDepth++;
        RecurseTriangles(lo, p, &leftBBox);
        mBspDepth--;
    } else {
        // We're done branching, so store a pointer to the list of triangles.
        mJSP->colltree.branchNodes[nodeIndex].leftInfo = CLUMPCOLL_MAKEINFO(kCLUMPCOLL_TRIANGLE, axis, lo);
    }

    if (!doneRight) {
        // Store a pointer to the right branch node.
        mJSP->colltree.branchNodes[nodeIndex].rightInfo = CLUMPCOLL_MAKEINFO(kCLUMPCOLL_BRANCH, axis, mJSP->colltree.branchNodes.size());

        // Shrink the bbox to the right region.
        RwBBox rightBBox = *bbox;
        SETCOORD(rightBBox.inf, axis, rightPlane);

        // Recurse down the right branch.
        mBspDepth++;
        RecurseTriangles(p + 1, hi, &rightBBox);
        mBspDepth--;
    } else {
        // We're done branching, so save a pointer to the list of triangles.
        mJSP->colltree.branchNodes[nodeIndex].rightInfo = CLUMPCOLL_MAKEINFO(kCLUMPCOLL_TRIANGLE, axis, p + 1);
    }

    // Now we delimit the left and right regions by marking their last triangles as not having a sibling.

    // If p < lo, that means there are no triangles in the left region.
    if (p >= lo) {
        mTriangles[p].bspTri.flags &= ~kCLUMPCOLL_HASNEXT;
    }

    // If p + 1 > hi, that means there are no triangles in the right region.
    if (p + 1 <= hi) {
        mTriangles[hi].bspTri.flags &= ~kCLUMPCOLL_HASNEXT;
    }
}

void JSPBuilder::CopyTriangles()
{
    mJSP->colltree.triangles.reserve(mTriangles.size());
    for (TriangleData& tri : mTriangles) {
        mJSP->colltree.triangles.push_back(tri.bspTri);
    }
}