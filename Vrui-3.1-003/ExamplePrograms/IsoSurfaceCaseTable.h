#ifndef ISOSURFACECASETABLE_H
#define ISOSURFACECASETABLE_H

class IsoSurfaceCaseTable
{
public:
    /* Elements: */
    static const int triangleEdgeIndices[256][16];
    static const int edgeMasks[256];
    static const int neighbourMasks[256];
};

#endif // ISOSURFACECASETABLE_H
