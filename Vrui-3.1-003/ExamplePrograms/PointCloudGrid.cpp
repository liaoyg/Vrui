#include "PointCloudGrid.h"
#include "string.h"
#include "math.h"


namespace visualization {

PointGridCell::PointGridCell(int x, int y, int z)
{
    indexX = x;
    indexY = y;
    indexZ = z;
}


PointCloudGrid::PointCloudGrid(int size)
    :gridSize(size),cubeCellNum(0)
{
    EleIntensityVolumeData = 0;
    testct = 100;
}

PointCloudGrid::~PointCloudGrid()
{
    delete EleIntensityVolumeData;
}

void PointCloudGrid::Initialization(Box bBox){
    cout<<"initialize grid"<<endl;
    boundingBox = bBox;
    lengthX = boundingBox.max[0]-boundingBox.min[0];
    lengthY = boundingBox.max[1]-boundingBox.min[1];
    lengthZ = boundingBox.max[2]-boundingBox.min[2];
    sizeX = gridSize;
    sizeY = lengthY*gridSize/lengthX;
    sizeZ = lengthZ*gridSize/lengthX;

    for(int i = 0;i < sizeZ; ++i)
        for(int j = 0;j < sizeY; ++j)
            for(int k = 0;k < sizeX; ++k)
            {
                PointGridCell* temp = new PointGridCell(k,j,i);
                GridCells.push_back(temp);
            }

    maxSize = sizeX > sizeY?(sizeX>sizeZ?sizeX:sizeZ):(sizeY>sizeZ?sizeY:sizeZ);
    EleIntensityVolumeData = new float[maxSize*maxSize*maxSize+1];
    memset(EleIntensityVolumeData,0,(maxSize*maxSize*maxSize+1)*sizeof(float));
}

void PointCloudGrid::ResizeVolumeData(int size)
{
    cout<<"Resize grid"<<endl;
    gridSize = size;
    sizeX = gridSize;
    sizeY = lengthY*gridSize/lengthX;
    sizeZ = lengthZ*gridSize/lengthX;

    GridCells.clear();
    for(int i = 0;i < sizeZ; ++i)
        for(int j = 0;j < sizeY; ++j)
            for(int k = 0;k < sizeX; ++k)
            {
                PointGridCell* temp = new PointGridCell(k,j,i);
                GridCells.push_back(temp);
            }
    maxSize = sizeX > sizeY?(sizeX>sizeZ?sizeX:sizeZ):(sizeY>sizeZ?sizeY:sizeZ);
    delete(EleIntensityVolumeData);
    EleIntensityVolumeData = new float[maxSize*maxSize*maxSize+1];

}

void PointCloudGrid::AddDensityNode(DensityNode *node){
    int xindex = (node->x-boundingBox.min[0])*sizeX/lengthX;
    if(xindex == sizeX) xindex--;
    int yindex = (node->y-boundingBox.min[1])*sizeY/lengthY;
    if(yindex == sizeY) yindex--;
    int zindex = (node->z-boundingBox.min[2])*sizeZ/lengthZ;
    if(zindex == sizeZ) zindex--;

    PointGridCell* temp = GridCells.at(xindex + yindex * sizeX + zindex * sizeY * sizeX);\
    if(temp != NULL){
        temp->pointPrList.push_back(node);
        pointNum++;

        int offset[3] = {-1,0,1};
        for(int x = 0; x<3;x++)
            for(int y = 0; y<3;y++)
                for(int z = 0; z<3;z++)
                {
                    if((xindex+offset[x]>0&&xindex+offset[x]<sizeX)&&(yindex+offset[y]>0&&yindex+offset[y]<sizeY)&&(zindex+offset[z]>0&&zindex+offset[z]<sizeZ))
                    {
                        int offsetx = xindex+offset[x];
                        int offsety = yindex+offset[y];
                        int offsetz = zindex+offset[z];
                        PointGridCell* gridCell = GridCells.at(offsetx+offsety*sizeX+offsetz*sizeX*sizeY);
                        gridCell->surroundingNodes.push_back(node);
                    }
                }
    }
}

bool IsNamePaired(string elename, vector<string> nameList)
{
    for(int i = 0; i < nameList.size(); ++i)
    {
        if(strcmp(elename.c_str(), nameList[i].c_str()) == 0)
            return true;
    }
    return false;
}

void PointCloudGrid::CalculateEleVolumeData(vector<string> elenameList)
{
//    int maxSize = PointCloudVis::FindMaxSizeofThree(sizeX,sizeY,sizeZ);
    memset(EleIntensityVolumeData,0,(maxSize*maxSize*maxSize+1)*sizeof(float));
    if(elenameList.size() == 0)
        return;

    for(int i = 0; i < GridCells.size(); ++i)
    {
        float intensity = 0;
        float totalValue = 0;
        for(int j = 0; j < GridCells[i]->pointPrList.size(); j++)
        {
            for(int k = 0; k < GridCells[i]->pointPrList[j]->intensityList.size(); ++k)
            {
                if(IsNamePaired(GridCells[i]->pointPrList[j]->intensityList[k].elementName, elenameList))
                    intensity += GridCells[i]->pointPrList[j]->intensityList[k].intensity;
            }
            totalValue += 1.0;
        }
        int x = GridCells[i]->indexX;
        int y = GridCells[i]->indexY;
        int z = GridCells[i]->indexZ;
        int offsetX = (maxSize-sizeX)/2;
        int offsetY = (maxSize-sizeY)/2;
        int offsetZ = (maxSize-sizeZ)/2;
        EleIntensityVolumeData[x + offsetX + (y+offsetY) * maxSize + (z+offsetZ) * maxSize * maxSize] = intensity/totalValue;
    }
}

float PointCloudGrid::InverseDistanceWeightFun(vector<DensityNode*> &nodelist, PointGridCell* cell,vector<string> &elementList)
{
    float centerX = (cell->indexX+0.5)*(lengthX/sizeX);
    float centerY = (cell->indexY+0.5)*(lengthY/sizeY);
    float centerZ = (cell->indexZ+0.5)*(lengthZ/sizeZ);
    float R = 1.5*sqrt(pow(lengthX/sizeX,2)+pow(lengthY/sizeY,2)+pow(lengthZ/sizeZ,2));
    float Rdfactor = 0.0;
    float intensity = 0;

    for(int i = 0; i < nodelist.size(); ++i)
    {
        DensityNode* node = nodelist[i];
        float distance = sqrt(pow(node->x-centerX,2)+pow(node->y-centerY,2)+pow(node->z-centerZ,2));
        Rdfactor += pow((R-distance)/(R*distance),2);
    }

    for(int i = 0; i < nodelist.size(); ++i)
    {
        DensityNode* node = nodelist[i];
        float distance = sqrt(pow(node->x-centerX,2)+pow(node->y-centerY,2)+pow(node->z-centerZ,2));
        for(int intCt = 0; intCt < node->intensityList.size(); ++intCt)
        {
            if(IsNamePaired(node->intensityList[intCt].elementName, elementList))
            {
                intensity += node->intensityList[intCt].intensity*
                        (pow((R-distance)/(R*distance),2)/Rdfactor);
            }
        }
    }
    return intensity;
}

float PointCloudGrid::BiCubicFactor(float x)
{
    float absx = fabs(x);
    float B = 1;
    float C = 0;
    float result;
    if(absx < 1)
        result = (12-9*B-6*C)*pow(absx,3)+(-18+12*B+6*C)*pow(absx,2)+(6-2*B);
    else if(absx < 2)
        result = (-B-6*C)*pow(absx,3)+(6*B+30*C)*pow(absx,2)+(-12*B-48*C)*absx+(8*B+24*C);
    else
        result = 0;
    result /=6;
    if(result == 0)
        cout<<"data error"<<endl;
    return result;
}

float PointCloudGrid::BiCubicDistanceWeightFun(vector<DensityNode *> &nodelist, PointGridCell *cell, vector<string> &elementList)
{
    float unitX = lengthX/sizeX;
    float unitY = lengthY/sizeY;
    float unitZ = lengthZ/sizeZ;
    float centerX = (cell->indexX+0.5)*(unitX)+boundingBox.min[0];
    float centerY = (cell->indexY+0.5)*(unitY)+boundingBox.min[1];
    float centerZ = (cell->indexZ+0.5)*(unitZ)+boundingBox.min[2];
    float intensity = 0;

    for(int i = 0; i < nodelist.size(); ++i)
    {
        DensityNode* node = nodelist[i];
        float filterX = BiCubicFactor((node->x-centerX)/unitX);
        float filterY = BiCubicFactor((node->y-centerY)/unitY);
        float filterZ = BiCubicFactor((node->z-centerZ)/unitZ);
        filterX = filterX>0?filterX:0;
        filterY = filterY>0?filterY:0;
        filterZ = filterZ>0?filterZ:0;
        for(int intCt = 0; intCt < node->intensityList.size(); ++intCt)
        {
            if(IsNamePaired(node->intensityList[intCt].elementName, elementList))
            {
                intensity += node->intensityList[intCt].intensity*
                        (filterX*filterY*filterZ);
            }
        }
    }
    return intensity;
}

void PointCloudGrid::CalculateEleVolumeDataWithFilter(vector<string> elenameList)
{
    std::cout<<"start cal volume with filter"<<std::endl;
//    memset(EleIntensityVolumeData,0,(maxSize*maxSize*maxSize+1)*sizeof(float));
    if(elenameList.size() == 0)
        return;

    for(int k = 0;k < sizeZ; ++k)
        for(int j = 0;j < sizeY; ++j)
            for(int i = 0;i < sizeX; ++i)
            {
                int cellIndex = i+j*sizeX+k*sizeX*sizeY;
                PointGridCell* grid = GridCells.at(cellIndex);

                int x = GridCells[cellIndex]->indexX;
                int y = GridCells[cellIndex]->indexY;
                int z = GridCells[cellIndex]->indexZ;
                int offsetX = (maxSize-sizeX)/2;
                int offsetY = (maxSize-sizeY)/2;
                int offsetZ = (maxSize-sizeZ)/2;
                EleIntensityVolumeData[x + offsetX + (y+offsetY) * maxSize + (z+offsetZ) * maxSize * maxSize] = BiCubicDistanceWeightFun(grid->surroundingNodes,grid,elenameList);
//                cout<<"surrrounding nodes size"<<x<<" "<<y<<" "<<z<<" "<<EleIntensityVolumeData[x + offsetX + (y+offsetY) * maxSize + (z+offsetZ) * maxSize * maxSize]<<endl;
            }
    std::cout<<"finish cal volume with filter"<<std::endl;
}

void PointCloudGrid::GridPointCloud(PointCloudVis *points)
{
    std::cout<<"start grid point cloud"<<std::endl;
    vector<DensityNode>& densityMap = points->GetDensityMap();
    for(int i = 0; i < densityMap.size(); ++i)
    {
        AddDensityNode(&densityMap[i]);
    }
}

void PointCloudGrid::GridCubeCells()
{
    for(int k = 0;k < sizeZ-1; ++k)
        for(int j = 0;j < sizeY-1; ++j)
            for(int i = 0;i < sizeX-1; ++i)
            {
                DataSetCell* cell = new DataSetCell;
                cell->addVertex(CalculateCellPosition(i,j,k));
                cell->addVertex(CalculateCellPosition(i+1,j,k));
                cell->addVertex(CalculateCellPosition(i,j,k+1));
                cell->addVertex(CalculateCellPosition(i+1,j,k+1));
                cell->addVertex(CalculateCellPosition(i,j+1,k));
                cell->addVertex(CalculateCellPosition(i+1,j+1,k));
                cell->addVertex(CalculateCellPosition(i,j+1,k+1));
                cell->addVertex(CalculateCellPosition(i+1,j+1,k+1));
                cell->adjustEdgePoint();
                cubeCellList.push_back(*cell);
                cubeCellNum++;
            }
}
Vertex PointCloudGrid::CalculateCellPosition(int x, int y, int z)
{
    int offsetX = (maxSize-sizeX)/2;
    int offsetY = (maxSize-sizeY)/2;
    int offsetZ = (maxSize-sizeZ)/2;
    float intensity = EleIntensityVolumeData[x + offsetX + (y+offsetY) * maxSize + (z+offsetZ) * maxSize * maxSize];
    Vertex vertex;
    vertex.x = (float)(x+offsetX)*2.0/maxSize - 1;
    vertex.y = (float)(y+offsetY)*2.0/maxSize - 1;
    vertex.z = (float)(z+offsetZ)*2.0/maxSize - 1;
    if(intensity > 0)
        vertex.intensity = intensity;
    else
        intensity = 0;
    return vertex;
}
}
