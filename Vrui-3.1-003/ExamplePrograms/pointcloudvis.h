#ifndef POINTCLOUDVIS_H
#define POINTCLOUDVIS_H

#include <vector>
#include <string>
#include <GL/GLColor.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Vrui/DisplayState.h>

using namespace std;

namespace visualization {


struct ElementNode
{
    string elementName;
    int elementNum;
    float intensity;
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

struct ElementIntensity
{
    string elementName;
    float intensity;
};

struct DensityNode
{
    float x,y,z;
    vector<ElementIntensity> intensityList;
};

struct VolumeDataNode
{
    string elementName;
    int volumeSizeX,volumeSizeY,volumeSizeZ;
    int maxSize;
    float* volumeData;
};

class PointCloudVis
{
public:
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Box<Scalar,3> Box;
    typedef string ElementName;
private:
    vector<IonNode> IonRangeList;
    vector<DensityNode> elementDensityMap;
    Box boundingBox;
//    float volumeData[128][128][128];
    vector<VolumeDataNode> volumeDataList;
    vector<string> elementList;
//    std::vector<unsigned char> volumeData;
    int pointVolumeSize;
    ElementName curElementName;

    void InputRangeFile(const char* filename);
    void LoadPosData(const char* filename);
    void GenerateVolumeData(const int size,const string elementName);
    void GenerateElementIndex();
public:
    PointCloudVis(const char* rangeFileName, const char* dataSrcFileName, int pointSize);
    virtual ~PointCloudVis(void);

    float* getVolumeDataPtr(const string elementName);
    int getVolumeSize(const string elementName);
    void RefreshVolumeData(const int pointSize);
    vector<string>& GetElementList();
    float* GetMultipleVolumeData(vector<string> elementlist);
    int FindMaxSizeofThree(int x, int y, int z);
    void SetCurrentElementName(ElementName name)
    {
        curElementName = name;
    }
    VolumeDataNode* getVolumeDataNode(const string elementName);
    Box getBoundingBox()
    {
        return boundingBox;
    }
    vector<DensityNode>& GetDensityMap();
};

}
#endif // POINTCLOUDVIS_H
