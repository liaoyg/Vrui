#include "DataFilter.h"
#include "pointcloudvis.h"

namespace visualization {

DataFilter::DataFilter()
    :filtedVolumeData(0)
{

}
int FindMaxofThree(int x, int y, int z)
{
    int max = x > y ? x:y;
    max = max > z ? max:z;
    return max;
}
void DataFilter::MedianFilter(float *volumedata, int maxSize)
{

//    int maxSize = FindMaxofThree(sizeX,sizeY,sizeZ);
    free(filtedVolumeData);
    filtedVolumeData = new float[maxSize*maxSize*maxSize+1];
    for(int i = 0; i < maxSize; i++)
        for(int j = 0; j < maxSize; j++)
            for(int k = 0; k < maxSize; k++)
            {
                vector<float> boxValue;
                int offset[3] = {-1,0,1};
                for(int x = 0; x<3;x++)
                    for(int y = 0; y<3;y++)
                        for(int z = 0; z<3;z++)
                        {
                            if((i+offset[x]>0&&i+offset[x]<maxSize)&&(j+offset[y]>0&&j+offset[y]<maxSize)&&(k+offset[z]>0&&k+offset[z]<maxSize))
                            {
                                int offseti = i+offset[x];
                                int offsetj = j+offset[y];
                                int offsetk = k+offset[z];
                                boxValue.push_back(volumedata[offseti + offsetj * maxSize + offsetk * maxSize * maxSize]);
                            }
                        }
                float median = FindMedian(boxValue,0,boxValue.size()-1,boxValue.size());
                filtedVolumeData[i + j * maxSize + k * maxSize * maxSize] = median;
            }
}

void exch(vector<float> &array, int i, int j)
{
    if(i != j)
    {
        float temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

float DataFilter::FindMedian(vector<float> &boxValue, int begin, int end, int n)
{
    if(boxValue.size() == 1)
        return boxValue[0];

    int i = begin;

    for(int j = i+1;j <= end; j++)
    {
        if(boxValue[j]<=boxValue[begin])
        {
            ++i;
            exch(boxValue,i,j);
        }
    }
    exch(boxValue,begin,i);

    if(i < n/2)
        return FindMedian(boxValue, i+1, end, n);
    else if(i > n/2)
        return FindMedian(boxValue, begin,i-1,n);
    else
    {
        if(n%2)
            return boxValue[i];
        else
        {
            int m =boxValue[0];
            for(int j = 1; j<i;++j)
            {
                if(boxValue[j] > m)
                    m = boxValue[j];
                return (float)(boxValue[i]+m)/2;
            }
        }
    }
}

}
