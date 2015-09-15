#ifndef POINTCLOUDGRID_H
#define POINTCLOUDGRID_H

#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include "pointcloudvis.h"
#include "DataSetCell.h"

namespace visualization {

struct PointGridCell
{
    int indexX;
    int indexY;
    int indexZ;
    vector<DensityNode*> pointPrList;
    vector<DensityNode*> surroundingNodes;

public:
    PointGridCell(int x, int y, int z);
};

class PointCloudGrid
{
public:
    typedef float Scalar;
    typedef Geometry::Box<Scalar,3> Box;
private:
    int gridSize;
    int sizeX;
    int sizeY;
    int sizeZ;
    int maxSize;
    float lengthX;
    float lengthY;
    float lengthZ;
    long int pointNum;
    string CurrentEle;
    float* EleIntensityVolumeData;
    Box boundingBox;
    vector<PointGridCell*> GridCells;

    vector<DataSetCell> cellList;

    int testct;

private:
    void AddDensityNode(DensityNode* node);
    float InverseDistanceWeightFun(vector<DensityNode*> &nodelist, PointGridCell* cell,vector<string> &elementList);
    float BiCubicFactor(float x);
    float BiCubicDistanceWeightFun(vector<DensityNode*> &nodelist, PointGridCell* cell,vector<string> &elementList);

public:
    PointCloudGrid(int size);
    ~PointCloudGrid();

    void Initialization(Box bBox);
    void CalculateEleVolumeData(vector<string> elenameList);
    void CalculateEleVolumeDataWithFilter(vector<string> elenameList);
    void GridPointCloud(PointCloudVis* points);
    void ResizeVolumeData(int size);
    int GetVolumeDataSize()
    {
        return maxSize;
    }

    float* GetCurVolumeData()
    {
        return EleIntensityVolumeData;
    }

};
}
#endif // POINTCLOUDGRID_H
