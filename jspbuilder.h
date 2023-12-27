#pragma once

#include "rw.h"
#include "jsp.h"

struct JSPBuilder
{
    void Build(JSP* jsp, RpClump* clump);

private:
    struct TriangleData
    {
        ClumpCollBSPTriangle bspTri;
        RwV3d* p;
        RwV3d min;
        RwV3d max;

        RwReal GetCenter(RwPlaneType axis) const
        {
            return (GETCOORD(min, axis) + GETCOORD(max, axis)) / 2.0f;
        }
    };

    struct Stats
    {
        RwInt32 maxDepthReached;
    };

    JSP* mJSP;
    RpClump* mClump;
    RwInt32 mBspDepth;
    Stats mStats;
    std::vector<TriangleData> mTriangles;

    void BuildJSPNodeList();
    void BuildStripVecList();
    void BuildBSPTree();

    void InitBBox(RwBBox* bbox);
    void InitTriangles();
    RwInt32 PartitionTriangles(RwInt32 lo, RwInt32 hi, RwReal splitPlane, RwPlaneType axis);
    void ChooseSplitPlane(RwBBox* bbox, RwInt32 lo, RwInt32 hi, RwReal* splitPlaneOut, RwPlaneType* axisOut);
    void RecurseTriangles(RwInt32 lo, RwInt32 hi, RwBBox* bbox);
    void CopyTriangles();
};