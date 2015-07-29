#include "pointcloudvis.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include <sstream>
#include <fstream>


using namespace std;

void PointCloudVis::InputRangeFile(const char *filename)
{
     ifstream rangeFileInput(filename);
     string readLine;
//     vector<IonNode> rangeFileList;
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
                 IonRangeList.push_back(ionTemp);
             }
         }
     }
}

void PointCloudVis::LoadPosData(const char* filename)
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

        elementDensityMap.push_back(tempNode);
    }while(!inputFile.eof());
}

int FindMaxSizeofThree(int x, int y, int z)
{
    int max = x > y ? x:y;
    max = max > z ? max:z;
    return max;
}

void PointCloudVis::GenerateVolumeData(const int size, const string elementName)
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

    VolumeDataNode tempNode;

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

    for(int i = 0; i < elementDensityMap.size(); i++)
    {
        x = (elementDensityMap[i].x-boundingBox.min[0])*(sizeX-1)/(boundingBox.max[0]-boundingBox.min[0]);
        y = (elementDensityMap[i].y-boundingBox.min[1])*(sizeY-1)/(boundingBox.max[1]-boundingBox.min[1]);
        z = (elementDensityMap[i].z-boundingBox.min[2])*(sizeZ-1)/(boundingBox.max[2]-boundingBox.min[2]);

        for(int j = 0; j < elementDensityMap[i].intensityList.size(); j++)
        {
            if(strcmp(elementDensityMap[i].intensityList[j].elementName.c_str(), elementName.c_str()) == 0)
                usefuleleNum[x + y * sizeX + z * sizeY * sizeX] += elementDensityMap[i].intensityList[j].intensity;
        }
            totaleleNum[x + y * sizeX + z * sizeY * sizeX] += 1.0f;
    }

    int nonZero = 0;
    int maxSize = FindMaxSizeofThree(sizeX,sizeY,sizeZ);
    float* volumeData = new float[maxSize*maxSize*maxSize+1];
    memset(volumeData,0,(maxSize*maxSize*maxSize+1)*sizeof(float));
    int offsetX = (maxSize-sizeX)/2;
    int offsetY = (maxSize-sizeY)/2;
    int offsetZ = (maxSize-sizeZ)/2;
    cout<<"offset size of volume: "<< offsetX<<" "<< offsetY<<" "<< offsetZ<<" "<<endl;

    for(int i = 0; i < sizeX; i++)
        for(int j = 0; j < sizeY; j++)
            for(int k = 0; k < sizeZ; k++)
            {

                float temp;
                if(totaleleNum[i + j * sizeX + k * sizeY * sizeX] == 0)
                    temp = 0.0f;
                else
                    temp = usefuleleNum[i + j * sizeX + k * sizeY * sizeX]/totaleleNum[i + j * sizeX + k * sizeY * sizeX];
                volumeData[i + offsetX + (j+offsetY) * maxSize + (k+offsetZ) * maxSize * maxSize] = temp;
//                volumeData[i + j * sizeX + k * sizeY * sizeX] = temp;
                if(temp > 0)
                    nonZero++;
            }
    //Put into volumeDataList
    tempNode.elementName = elementName;
    tempNode.volumeData = volumeData;
    tempNode.volumeSizeX = sizeX;
    tempNode.volumeSizeY = sizeY;
    tempNode.volumeSizeZ = sizeZ;
    tempNode.maxSize = maxSize;
    volumeDataList.push_back(tempNode);
    cout<<"nonzero: "<<nonZero<<endl;
}\

void PointCloudVis::GenerateElementIndex()
{
    if(elementDensityMap.size() == 0)
        return;

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
}

PointCloudVis::PointCloudVis(const char* rangeFileName, const char* dataSrcFileName, int pointSize)
{

    pointVolumeSize = pointSize;
    boundingBox = Box(Point(0,0,0),Point(1,1,1));

    InputRangeFile(rangeFileName);
    LoadPosData(dataSrcFileName);
    GenerateElementIndex();

    for(int i = 0; i< elementList.size();i++)
    {
        cout<<"generate volume data, element: "<<elementList[i]<<endl;
        GenerateVolumeData(pointSize,elementList[i]);
    }

}

PointCloudVis::~PointCloudVis()
{
    for(int i =0 ;i < volumeDataList.size();i++)
        if(volumeDataList[i].volumeData != NULL)
            free(volumeDataList[i].volumeData);
}

float* PointCloudVis::getVolumeDataPtr(const string elementName)
{
    for(int i = 0 ; i < volumeDataList.size(); i++)
    {
        if(strcmp(volumeDataList[i].elementName.c_str(), elementName.c_str()) == 0)
            return volumeDataList[i].volumeData;
    }
}

int PointCloudVis::getVolumeSize(const string elementName)
{
    for(int i = 0 ; i < volumeDataList.size(); i++)
    {
        if(strcmp(volumeDataList[i].elementName.c_str(), elementName.c_str()) == 0)
        {
            int maxSize = volumeDataList[i].volumeSizeX > volumeDataList[i].volumeSizeY ? volumeDataList[i].volumeSizeX:volumeDataList[i].volumeSizeY;
            maxSize = maxSize > volumeDataList[i].volumeSizeZ ? maxSize : volumeDataList[i].volumeSizeZ;
            return maxSize;
        }
    }
}

void PointCloudVis::RefreshVolumeData(const int pointSize)
{
    if(pointSize != pointVolumeSize)
    {
        for(int i = 0; i< elementList.size();i++)
        {
            cout<<"generate volume data, element: "<<elementList[i]<<endl;
            GenerateVolumeData(pointSize,elementList[i]);
        }
    }
}
