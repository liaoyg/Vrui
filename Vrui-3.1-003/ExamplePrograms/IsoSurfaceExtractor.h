#ifndef ISOSURFACEEXTRACTOR_H
#define ISOSURFACEEXTRACTOR_H

#include "PointCloudGrid.h"
#include "pointcloudvis.h"
#include "IsoSurfaceCaseTable.h"
#include "IndexedTriangleSet.h"

#include <Misc/HashTable.h>
#include <Misc/OneTimeQueue.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Vector.h>

namespace visualization {
class IndexedTriangleSet;
}

namespace visualization {


class IsoSurfaceExtractor
{
public:
    enum ExtractionMode
    {
        FLAT,
        SMOOTH
    };
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Box<Scalar,3> Box;
    typedef Geometry::Vector<double,3> Vector;

    typedef IndexedTriangleSet Isosurface; // Type of isosurface representation
    typedef typename Isosurface::Vertex Vertex; // Type of vertices stored in isosurface
    typedef typename Isosurface::Index Index; // Type for vertex indices
    typedef IsoSurfaceCaseTable CaseTable;
//    typedef Misc::HashTable<EdgeID,Index,EdgeID> VertexIndexHasher;

private:
    ExtractionMode extractionmode;
    PointCloudGrid* dataset;
    Isosurface* isosurface;
    Scalar isoValue;
//    VertexIndexHasher vertexIndices; // Hasher mapping edge IDs to vertex indices in the isosurface


public:
    IsoSurfaceExtractor(PointCloudGrid* sDataSet);
    ~IsoSurfaceExtractor();
    void SetNewDataSetGrid(PointCloudGrid* sDataSet);

    void ExtractIsoSurface(const Scalar newIsoValue, Isosurface* newIsoSurface);
    Isosurface* getSurface()
    {
        return isosurface;
    }

private:
    void ExtractFlatIsoSurface( DataSetCell& cell);
    void ExtractSmoothIsoSurface( DataSetCell& cell);
};

}
#endif // ISOSURFACEEXTRACTOR_H
