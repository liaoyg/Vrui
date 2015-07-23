#ifndef POINTCLOUDVIS_H
#define POINTCLOUDVIS_H

#include <vector>
#include <string>
#include <GL/GLColor.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Vrui/DisplayState.h>


using namespace std;

struct ElementNode
{
    string elementName;
    int elementNum;
};

struct IonNode
{
    float minMass;
    float maxMass;
    float volume;
    vector<ElementNode> elementList;
    typedef GLColor<GLfloat,4> ionColor;
    ionColor color;
};

struct DensityNode
{
    float x,y,z;
    float intensity;
};

class PointCloudVis
{
public:
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Box<Scalar,3> Box;
private:
    vector<IonNode> IonRangeList;
    vector<DensityNode> CaDensityMap;
    Box boundingBox;
//    float volumeData[128][128][128];
    float* volumeData;
//    std::vector<unsigned char> volumeData;
    int pointVolumeSize;

    vector<IonNode> InputRangeFile(const char* filename);
    void LoadPosData(const char* filename, const string eleName, vector<DensityNode>& nodeList, Box& boundingBox);
    void GenerateVolumeData(const int size);
public:
    PointCloudVis(const char* rangeFileName, const char* dataSrcFileName, int pointSize);
    virtual ~PointCloudVis(void);

    float* getVolumeDataPtr();
};

#endif // POINTCLOUDVIS_H
