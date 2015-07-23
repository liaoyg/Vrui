#include "pointcloudvis.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include <sstream>
#include <fstream>


using namespace std;

 vector<IonNode> PointCloudVis::InputRangeFile(const char *filename)
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
                rangeFileList.push_back(ionTemp);
            }
        }
    }
    return rangeFileList;
}

void PointCloudVis::LoadPosData(const char* filename, const string eleName, vector<DensityNode>& nodeList, Box& boundingBox)
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
                int eleNum = 0;
                int totalEleNum = 0;
                for(int j = 0; j < IonRangeList[i].elementList.size(); j++)
                {
                    if(IonRangeList[i].elementList[j].elementName == eleName)
                    {
                        eleNum += IonRangeList[i].elementList[j].elementNum;
                        totalEleNum += IonRangeList[i].elementList[j].elementNum;
                    }
                    else
                    {
                        totalEleNum += IonRangeList[i].elementList[j].elementNum;
                    }
                }
                tempNode.intensity = (float)eleNum/totalEleNum;
                break;
            }
        }

        nodeList.push_back(tempNode);
    }while(!inputFile.eof());
}

void PointCloudVis::GenerateVolumeData(const int size)
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
    for(int i = 0; i < CaDensityMap.size(); i++)
    {
        x = (CaDensityMap[i].x-boundingBox.min[0])*size/(boundingBox.max[0]-boundingBox.min[0]);
        y = (CaDensityMap[i].y-boundingBox.min[1])*size/(boundingBox.max[1]-boundingBox.min[1]);
        z = (CaDensityMap[i].z-boundingBox.min[2])*size/(boundingBox.max[2]-boundingBox.min[2]);
//        cout<<CaDensityMap[i].x<<" min:"<<boundingBox.min[0]<<" "<<boundingBox.max[0]<<endl;
//        cout<<x<<" "<<y<<" "<<z<<" "<<CaDensityMap[i].intensity<<endl;
        usefuleleNum[x][y][z] += CaDensityMap[i].intensity;
        totaleleNum[x][y][z] += 1.0f;
    }
    volumeData = (float *)malloc(size*size*size*sizeof(float));
//    volumeData.resize(size*size*size);
    int nonZero = 0;
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            for(int k = 0; k < size; k++)
            {
                float temp;
                if(totaleleNum[i][j][k] == 0)
                    temp = 0.0f;
                else
                    temp = usefuleleNum[i][j][k]/totaleleNum[i][j][k];
                volumeData[i+j*size+k*size*size] = temp;
//                if(totaleleNum[i][j][k] != 0)
//                    nonZero++;
//                cout<<i<<","<<j<<","<<k<<":"<<usefuleleNum[i][j][k]<<" "<<totaleleNum[i][j][k]<<" "<<temp<<"nonZero: "<<nonZero<<endl;
            }

}

PointCloudVis::PointCloudVis(const char* rangeFileName, const char* dataSrcFileName, int pointSize)
{
//    for(int i=1;i<argc;++i)
//        {
//        /* Check if the current command line argument is a switch or a file name: */
//        if(argv[i][0]=='-')
//            {
//            /* Determine the kind of switch and change the file mode: */
//            if(strcasecmp(argv[i]+1,"range")==0)
//            {
//                ++i;
//                if(i<argc)
//                    rangeFileName = argv[i];
//            }
//            else if(strcasecmp(argv[i]+1,"pos")==0)
//            {
//                ++i;
//                if(i<argc)
//                    dataSrcFileName = argv[i];
//            }
//        }
//    }
    pointVolumeSize = pointSize;
    boundingBox = Box(Point(0,0,0),Point(1,1,1));
    IonRangeList = InputRangeFile(rangeFileName);
//    for(int i = 0; i < IonRangeList.size(); i++)
//    {
//        cout<<IonRangeList[i].minMass<<"~"<<IonRangeList[i].maxMass<<" ";
//        for(int j = 0; j < IonRangeList[i].elementList.size();j++)
//            cout<<IonRangeList[i].elemeunsigned charntList[j].elementName<<":"<<IonRangeList[i].elementList[j].elementNum<<" "<<"Color:";
//        for(int j = 0; j< 4; j++)
//            cout<<" "<<IonRangeList[i].color[j];
//        cout<<endl;
//    }

    LoadPosData(dataSrcFileName, "Na", CaDensityMap, boundingBox);
//    for(int i = 0; i < CaDensityMap.size(); i++)
//    {
//        cout<<CaDensityMap[i].x<<" "<<CaDensityMap[i].y<<" "<<CaDensityMap[i].z<<" "<<CaDensityMap[i].intensity<<endl;
//    }
//    for(int i = 0; i < 3; i++)
//    {
//        cout<<boundingBox.min[i]<<" ";
//    }
//    cout<<endl;
//    for(int i = 0; i < 3; i++)
//    {
//        cout<<boundingBox.max[i]<<" ";
//    }
//    cout<<endl;

    cout<<"generate volume"<<endl;
    GenerateVolumeData(pointVolumeSize);
    for(int i = 0; i < pointVolumeSize*pointVolumeSize*pointVolumeSize; i++)
        cout<<volumeData[i]<<endl;
}

PointCloudVis::~PointCloudVis()
{
    free(volumeData);
}

float* PointCloudVis::getVolumeDataPtr()
{
    return volumeData;
}
