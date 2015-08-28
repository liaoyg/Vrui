#ifndef DATASETGRID_H
#define DATASETGRID_H


#include "DataSetCell.h"

namespace visualization {
struct VolumeDataNode;
}
namespace visualization {

class DataSetGrid
{
public:
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Box<Scalar,3> Box;
    typedef vector<DataSetCell>::iterator cellitr;
    typedef visualization::VolumeDataNode DataNode;
private:
    vector<DataSetCell> cellList;
    size_t cellNum;
public:
    DataSetGrid();
    ~DataSetGrid();

    void Initialization(DataNode* vertexData);
    size_t getTotalNumCells()
    {
        return cellNum;
    }
    cellitr beginCells()
    {
        return cellList.begin();
    }
    cellitr lastCells()
    {
        return cellList.end();
    }

private:
    Vertex CalculateVertex(int x, int y, int z, DataNode* vertexData);
};

}

#endif // DATASETGRID_H
