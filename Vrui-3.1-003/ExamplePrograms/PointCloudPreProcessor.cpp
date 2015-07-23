#include "stdlib.h"
#include "string.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
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

struct DensityNode
{
    float x,y,z;
    vector<ElementIntensity> intensityList;
};

struct ElementIntensity
{
    string elementName;
    float intensity;
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

void LoadPosData(const char* filename, vector<DensityNode>& nodeList, Box& boundingBox)
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


        for(int i = 0; i < IonRangeList.size(); i++)
        {
            if(fvalue[3] >= IonRangeList[i].minMass && fvalue[3] <= IonRangeList[i].maxMass)
            {
                for(int j = 0; j < IonRangeList[i].elementList.size(); j++)
                {
                    ElementIntensity tempEleInt;
                    tempEleInt.elementName = IonRangeList[i].elementList[j].elementName;
                    tempEleInt.intensity = IonRangeList[i].elementList[j].intensity;
                    tempNode.intensityList.push_back(tempEleInt);
                }
                break;
            }
        }

        nodeList.push_back(tempNode);
    }while(!inputFile.eof());
}


void GenerateVolumeData(const int size, vector<DensityNode> densityMap, const string elementName, const string outputPath)
{
    int x,y,z;
    float totaleleNum[size][size][size];
    float usefuleleNum[size][size][size];
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            for(int k = 0; k < size; k++)
            {
                totaleleNum[i][j][k] = 0;
                usefuleleNum[i][j][k] = 0;
            }
//    cout<<CaDensityMap.size()<<" ca size"<<endl;
    for(int i = 0; i < densityMap.size(); i++)
    {
        x = (densityMap[i].x-boundingBox.min[0])*size/(boundingBox.max[0]-boundingBox.min[0]);
        y = (densityMap[i].y-boundingBox.min[1])*size/(boundingBox.max[1]-boundingBox.min[1]);
        z = (densityMap[i].z-boundingBox.min[2])*size/(boundingBox.max[2]-boundingBox.min[2]);
//        cout<<CaDensityMap[i].x<<" min:"<<boundingBox.min[0]<<" "<<boundingBox.max[0]<<endl;
//        cout<<x<<" "<<y<<" "<<z<<" "<<CaDensityMap[i].intensity<<endl;
        for(int j = 0; j < densityMap[i].intensityList.size(); j++)
        {
            if(strcmp(densityMap[i].intensityList[j].elementName.c_str(), elementName.c_str()))
                usefuleleNum[x][y][z] += densityMap[i].intensityList[j].intensity;
            totaleleNum[x][y][z] += 1.0f;
        }
    }
    volumeData = (float *)malloc(size*size*size*sizeof(float));
//    volumeData.resize(size*size*size);
    int nonZero = 0;

    ofstream output;
    string outputFilePath = outputPath+"/"+elementName + ".raw";
    output.open(outputFilePath.c_str());

    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            for(int k = 0; k < size; k++)
            {
                float temp;
                if(totaleleNum[i][j][k] == 0)
                    temp = 0.0f;
                else
                    temp = usefuleleNum[i][j][k]/totaleleNum[i][j][k];
//                volumeData[i+j*size+k*size*size] = temp;
                output<<setprecision(6)<<temp;
            }

}

int main(int argc,char* argv[])
{
    const char* rangeFileName=0;
    const char* dataSrcFileName = 0;

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
        }
    }
    Box boundingBox = Box(Point(0,0,0),Point(1,1,1));
    vector<IonNode> rangeFile = InputRangeFile(rangeFileName);
    vector<DensityNode> elementDensityMap;
    LoadPosData(dataSrcFileName,elementDensityMap, boundingBox);



}
