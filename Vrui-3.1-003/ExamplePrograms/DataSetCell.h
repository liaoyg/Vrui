#ifndef DATASETCELL_H
#define DATASETCELL_H

#include <vector>
#include <iostream>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Vector.h>

using namespace std;

struct Vertex
{
    float x,y,z;
    float intensity;
};

class DataSetCell
{
public:
    enum GridMode
    {
        Simplex,
        Tesseract
    };
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Vector<Scalar,3> Vector;
    typedef Geometry::Box<Scalar,3> Box;

    static const int edgeNum = 12;
private:
    int verticesNum;
    vector<Vertex> vertices;
    Vertex* edgeVertices[12][2];
    int edgeVertexIndice[12][2]=
    {{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};

public:
    DataSetCell();

    void addVertex(Vertex vertex);
    void adjustEdgePoint();
    int getVerticesNum()
    {
        return verticesNum;
    }
    float getVerticesValue(int index);
    float getEdgeEndPointValue(const int edge, const int pointindex);
    Point calEdgePointPosition(const int edgeindex, float value);
};

#endif // DATASETCELL_H
