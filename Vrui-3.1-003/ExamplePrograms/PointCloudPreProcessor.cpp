#include "stdlib.h"
#include "string.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <GL/GLColor.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>

using namespace std;

typedef float Scalar;
typedef Geometry::Point<Scalar,2> Point2D;
typedef Geometry::Point<Scalar,3> Point;
typedef Geometry::Box<Scalar,3> Box;

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

 vector<IonNode> InputRangeFile(const char *filename)
{
    ifstream rangeFileInput(filename);
    string readLine;
    vector<IonNode> rangeFileList;
    while(rangeFileInput.good())
    {
        getline(rangeFileInput, readLine);

        if(strcmp(readLine.c_str(), "[Ranges]\r") == 0)
        {
            int rangeNum = 0;
            getline(rangeFileInput, readLine);
            rangeNum = atoi(readLine.substr(7,2).c_str());
            for(int i = 0; i< rangeNum; i++)
            {
                //input rangefile data
                string dataline;
                getline(rangeFileInput, dataline);
                IonNode ionTemp;
                int start = 0;
                int end = dataline.find_first_of(" ", start) - 1;
                while(end != -2)
                {
                    int pos = 0;
                    if(start == 0)
                    {
                        pos = dataline.find("=", start);
                        ionTemp.minMass = atof(dataline.substr(pos+1,end-pos).c_str());
                        start = end+2;
                        end = dataline.find_first_of(" ", start) - 1;
                        ionTemp.maxMass = atof(dataline.substr(start,end-start).c_str());
                    }
                    else
                    {
                        pos = dataline.find(":", start);
                        if(strcmp(dataline.substr(start, pos - start).c_str(),"Vol") == 0)
                        {
                            ionTemp.volume = atof(dataline.substr(pos+1, end - pos).c_str());
                        }
                        else
                        {
                            vector<ElementNode>::iterator it;
                            string eleName = dataline.substr(start, pos-start);
                            int eleNum = atoi(dataline.substr(pos+1, end-pos).c_str());
                            bool isFound = false;
                            for(it = ionTemp.elementList.begin(); it != ionTemp.elementList.end(); it++)
                            {
                                if(strcmp((*it).elementName.c_str(), eleName.c_str())==0)
                                {
                                    (*it).elementNum += eleNum;
                                    isFound = true;
                                    break;
                                }
                            }
                            if(!isFound)
                            {
                                ElementNode nodeTemp;
                                nodeTemp.elementName = eleName;
                                nodeTemp.elementNum = eleNum;
                                ionTemp.elementList.push_back(nodeTemp);
                            }
                        }
                    }
                    start = end+2;
                    end = dataline.find_first_of(" ", start) - 1;
                }
                //Color is the last column
                int pos = dataline.find(":",start);
                if(strcmp(dataline.substr(start, pos-start).c_str(), "Color") == 0)
                {
                    char *str;
                    ionTemp.color[0] = (float)strtol(dataline.substr(pos+1,2).c_str(), &str, 16)/255;
                    ionTemp.color[1] = (float)strtol(dataline.substr(pos+3,2).c_str(), &str, 16)/255;
                    ionTemp.color[2] = (float)strtol(dataline.substr(pos+5,2).c_str(), &str, 16)/255;
                }
                //calculate element intensity
                int totalEleNum = 0;
                for(int j = 0; j< ionTemp.elementList.size(); j++)
                    totalEleNum += ionTemp.elementList[j].elementNum;
                for(int j = 0; j< ionTemp.elementList.size(); j++)
                    ionTemp.elementList[j].intensity = (float)ionTemp.elementList[j].elementNum/totalEleNum;
                rangeFileList.push_back(ionTemp);
            }
        }
    }
    return rangeFileList;
}

void LoadPosData(const char* filename, vector<DensityNode>& nodeList, vector<IonNode>& rangeFileList, Box& boundingBox)
{
    ifstream inputFile;
    inputFile.open(filename);

    char inputbuffer[16];

    do
    {
        inputFile.read(inputbuffer, 16);
        float fvalue[4];
        unsigned char* intermedTem = NULL;
        for(int i = 0; i < 4; i++)
        {
            intermedTem = (unsigned char*)&fvalue[i];
            for(int k = 0; k < 4; k++)
            {
                intermedTem[k] = inputbuffer[i*4+3-k];
            }
        }
        DensityNode tempNode;
        tempNode.x = fvalue[0];
        tempNode.y = fvalue[1];
        tempNode.z = fvalue[2];

        for(int i = 0; i < 3; i++)
        {
            if(fvalue[i]<boundingBox.min[i])
                boundingBox.min[i] = fvalue[i];
            else if(fvalue[i]>boundingBox.max[i])
                boundingBox.max[i] = fvalue[i];
        }


        for(int i = 0; i < rangeFileList.size(); i++)
        {
            if(fvalue[3] >= rangeFileList[i].minMass && fvalue[3] <= rangeFileList[i].maxMass)
            {
                for(int j = 0; j < rangeFileList[i].elementList.size(); j++)
                {
                    ElementIntensity tempEleInt;
                    tempEleInt.elementName = rangeFileList[i].elementList[j].elementName;
                    tempEleInt.intensity = rangeFileList[i].elementList[j].intensity;
                    tempNode.intensityList.push_back(tempEleInt);
                }
                break;
            }
        }

        nodeList.push_back(tempNode);
    }while(!inputFile.eof());
}


void GenerateVolumeDataAlt(const int size, vector<DensityNode>& densityMap, Box& boundingBox, const string elementName, const string outputPath)
{
    float lengthX = boundingBox.max[0]-boundingBox.min[0];
    float lengthY = boundingBox.max[1]-boundingBox.min[1];
    float lengthZ = boundingBox.max[2]-boundingBox.min[2];
    int sizeX = size;
    int sizeY = lengthY*size/lengthX;
    int sizeZ = lengthZ*size/lengthX;

    ofstream output;
    string outputFilePath = outputPath + elementName + ".raw";
    output.open(outputFilePath.c_str());

    for(int i = 0; i< sizeX; i++)
        for(int j = 0; j< sizeY; j++)
            for(int k = 0; k< sizeX; k++)
            {
                float elementIntAcum = 0.0f;
                float totalIntAcum = 0.0f;
                for(int n = 0; n < densityMap.size(); n++)
                {

                    if((densityMap[n].x >= (float)i*lengthX/sizeX && densityMap[n].x < (float)(i+1)*lengthX/sizeX)
                       && (densityMap[n].y >= (float)j*lengthY/sizeY && densityMap[n].y < (float)(j+1)*lengthY/sizeY)
                       && (densityMap[n].z >= (float)k*lengthZ/sizeZ && densityMap[n].z < (float)(k+1)*lengthZ/sizeZ))
                    {
                        totalIntAcum += 1.0f;
                        for(int m = 0; m < densityMap[n].intensityList.size(); m++)
                        {
                            if(strcmp(densityMap[n].intensityList[m].elementName.c_str(), elementName.c_str()))
                                elementIntAcum += densityMap[n].intensityList[m].intensity;
                        }
                    }

                }

                float temp;
                if(totalIntAcum == 0)
                    temp = 0.0f;
                else
                    temp = elementIntAcum/totalIntAcum;
                output<<setprecision(6)<<temp;
            }
}

void GenerateVolumeData(const int size, vector<DensityNode>& densityMap, Box& boundingBox, const string elementName, const string outputPath)
{
    const float lengthX = boundingBox.max[0]-boundingBox.min[0];
    const float lengthY = boundingBox.max[1]-boundingBox.min[1];
    const float lengthZ = boundingBox.max[2]-boundingBox.min[2];
    const int sizeX = size;
    const int sizeY = lengthY*size/lengthX;
    const int sizeZ = lengthZ*size/lengthX;
//    const int sizeY = size;
//    const int sizeZ = size;
    cout<<"dataset size: "<<sizeX<<" "<<sizeY<<" "<<sizeZ<<endl;
    int x,y,z;
    float* totaleleNum;
    float* usefuleleNum;
    usefuleleNum = new float[sizeX*sizeY*sizeZ+1];
    totaleleNum = new float[sizeX*sizeY*sizeZ+1];

    for(int i = 0; i < sizeX; i++)
        for(int j = 0; j < sizeY; j++)
            for(int k = 0; k < sizeZ; k++)
            {
                totaleleNum[i + j * sizeX + k * sizeY * sizeX] = 0;
                totaleleNum[i + j * sizeX + k * sizeY * sizeX]  = 0;
            }

    for(int i = 0; i < densityMap.size(); i++)
    {
        x = (densityMap[i].x-boundingBox.min[0])*(sizeX-1)/(boundingBox.max[0]-boundingBox.min[0]);
        y = (densityMap[i].y-boundingBox.min[1])*(sizeY-1)/(boundingBox.max[1]-boundingBox.min[1]);
        z = (densityMap[i].z-boundingBox.min[2])*(sizeZ-1)/(boundingBox.max[2]-boundingBox.min[2]);

        for(int j = 0; j < densityMap[i].intensityList.size(); j++)
        {
            if(strcmp(densityMap[i].intensityList[j].elementName.c_str(), elementName.c_str()))
                usefuleleNum[x + y * sizeX + z * sizeY * sizeX] += densityMap[i].intensityList[j].intensity;
        }
            totaleleNum[x + y * sizeX + z * sizeY * sizeX] += 1.0f;
    }
//    volumeData = (float *)malloc(size*size*size*sizeof(float));
//    volumeData.resize(size*size*size);
    int nonZero = 0;

    ofstream output;
    string outputFilePath = outputPath + elementName + ".raw";
    output.open(outputFilePath.c_str());

    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            for(int k = 0; k < size; k++)
            {
                float temp;
                if(totaleleNum[i + j * sizeX + k * sizeY * sizeX] == 0)
                    temp = 0.0f;
                else
                    temp = usefuleleNum[i + j * sizeX + k * sizeY * sizeX]/totaleleNum[i + j * sizeX + k * sizeY * sizeX];
//                volumeData[i+j*size+k*size*size] = temp;
                if(temp > 0)
                {
//                    cout<<temp<<endl;
                    nonZero++;
                }
//                output<<temp;
                output.write((char*)(&temp), sizeof(float));
            }
    cout<<"nonzero: "<<nonZero<<endl;
    cout<<"output volume data: "<<outputFilePath<<endl;
}\

vector<string> outputElementIndex(const string outputPath, vector<DensityNode>& elementDensityMap)
{
    vector<string> elementList;
    if(elementDensityMap.size() == 0)
        return elementList;

    for(int i = 0; i < elementDensityMap.size(); i++)
        for(int k = 0; k < elementDensityMap[i].intensityList.size(); k++)
        {
            bool isFound = false;
            for(int j =0 ;j < elementList.size();j++)
            {
                if(strcmp(elementDensityMap[i].intensityList[k].elementName.c_str(), elementList[j].c_str())==0)
                    isFound = true;
            }
            if(!isFound)
                elementList.push_back(elementDensityMap[i].intensityList[k].elementName);
        }

    ofstream output;
    string outputFilePath = outputPath + "element.index";
    output.open(outputFilePath.c_str());

    for(int i = 0; i < elementList.size(); i++)
    {
        output<<elementList[i]<<endl;
    }

    return elementList;
}

int main(int argc,char* argv[])
{
    const char* rangeFileName = 0;
    const char* dataSrcFileName = 0;
    const char* outputPath = 0;

    for(int i=1;i<argc;++i)
        {
        /* Check if the current command line argument is a switch or a file name: */
        if(argv[i][0]=='-')
            {
            /* Determine the kind of switch and change the file mode: */
            if(strcasecmp(argv[i]+1,"range")==0)
            {
                ++i;
                if(i<argc)
                    rangeFileName = argv[i];
            }
            else if(strcasecmp(argv[i]+1,"pos")==0)
            {
                ++i;
                if(i<argc)
                    dataSrcFileName = argv[i];
            }
            else if(strcasecmp(argv[i]+1,"o")==0)
            {
                ++i;
                if(i<argc)
                    outputPath = argv[i];
            }
        }
    }
    Box boundingBox = Box(Point(0,0,0),Point(1,1,1));
    vector<IonNode> rangeFile = InputRangeFile(rangeFileName);
    vector<DensityNode> elementDensityMap;
    LoadPosData(dataSrcFileName,elementDensityMap, rangeFile, boundingBox);

    //output element Index
    vector<string> elementList = outputElementIndex(outputPath,elementDensityMap);

    //output volume data file
    for(int i = 0; i < elementList.size(); i++)
        GenerateVolumeData(2, elementDensityMap, boundingBox, elementList[i], outputPath);
//    GenerateVolumeData(128, elementDensityMap, boundingBox, "Na", outputPath);

}
