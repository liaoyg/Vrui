#include "DataSetGrid.h"
#include "pointcloudvis.h"

namespace visualization {

DataSetGrid::DataSetGrid()
    :cellList(0),cellNum(0)
{
}

DataSetGrid::~DataSetGrid()
{
    cellList.clear();
}

void DataSetGrid::Initialization(DataNode* vertexData)
{
    for(int i = 0; i < vertexData->volumeSizeX-1; i++)
        for(int j = 0; j < vertexData->volumeSizeY-1; j++)
            for(int k = 0; k < vertexData->volumeSizeZ-1; k++)
            {
                //create grid cell for each point
                //each point start as index 0 of the cell
                DataSetCell* cell = new DataSetCell;
                cell->addVertex(CalculateVertex(i,j,k,vertexData));
                cell->addVertex(CalculateVertex(i+1,j,k,vertexData));
                cell->addVertex(CalculateVertex(i,j,k+1,vertexData));
                cell->addVertex(CalculateVertex(i+1,j,k+1,vertexData));
                cell->addVertex(CalculateVertex(i,j+1,k,vertexData));
                cell->addVertex(CalculateVertex(i+1,j+1,k,vertexData));
                cell->addVertex(CalculateVertex(i,j+1,k+1,vertexData));
                cell->addVertex(CalculateVertex(i+1,j+1,k+1,vertexData));
                cell->adjustEdgePoint();
                cellList.push_back(*cell);
                cellNum++;
            }
    std::cout<<"Cell Num: "<<cellNum<<std::endl;
}

Vertex DataSetGrid::CalculateVertex(int i, int j, int k, DataNode* vertexData)
{
    float intensity = vertexData->volumeData[i + j * vertexData->volumeSizeX + k * vertexData->volumeSizeY * vertexData->volumeSizeX];
    Vertex vertex;
    vertex.x = (float)i*2/(vertexData->volumeSizeX-1) - 1;
    vertex.y = (float)j*2/(vertexData->volumeSizeY-1) - 1;
    vertex.z = (float)k*2/(vertexData->volumeSizeZ-1) - 1;
    if(intensity > 0)
        vertex.intensity = intensity;
    else
        intensity = 0;
    return vertex;
}

}
