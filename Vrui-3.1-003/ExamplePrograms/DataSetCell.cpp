#include "DataSetCell.h"

DataSetCell::DataSetCell()
    :verticesNum(0)
{

}

void DataSetCell::addVertex(Vertex vertex)
{
    vertices.push_back(vertex);
    verticesNum++;
}

void DataSetCell::adjustEdgePoint()
{
    if(verticesNum != 8)
    {
        cout<<"cell initialization faild, missing vertices"<<endl;
        return;
    }
    for(int i = 0 ; i < 12; i++)
        for(int j = 0 ; j < 2; j++)
        {
            edgeVertices[i][j] = &vertices[edgeVertexIndice[i][j]];
        }
}

float DataSetCell::getVerticesValue(int index)
{
    Vertex vertex = vertices.at(index);
    return vertex.intensity;
}

float DataSetCell::getEdgeEndPointValue(const int edge, const int pointindex)
{
    Vertex* vertex = edgeVertices[edge][pointindex];
    return vertex->intensity;
}

DataSetCell::Point DataSetCell::calEdgePointPosition(const int edgeindex, float value)
{
    Vertex* vertex0 = edgeVertices[edgeindex][0];
    Vertex* vertex1 = edgeVertices[edgeindex][1];
//    std::cout<<"w1 value"<<value<<"vertex0: "<<vertex0->x<<" "<<vertex0->y<<" "<<vertex0->z<<"vertex1: "<<vertex1->x<<" "<<vertex1->y<<" "<<vertex1->z<<endl;
    Scalar x = (vertex1->x-vertex0->x)*value+vertex0->x;
    Scalar y = (vertex1->y-vertex0->y)*value+vertex0->y;
    Scalar z = (vertex1->z-vertex0->z)*value+vertex0->z;
    Point result = Point(x,y,z);
    return result;
}
