#ifndef DATAFILTER_H
#define DATAFILTER_H

#include <vector>

using namespace std;

namespace visualization {

struct VolumeDataNode;

class DataFilter
{
private:
    float* filtedVolumeData;

    float FindMedian(vector<float> &boxValue, int begin, int end, int n);

public:
    DataFilter();

    void MedianFilter(float* volumedata, int maxSize);

    float* GetFiltedData()
    {
        return filtedVolumeData;
    }

};
}
#endif // DATAFILTER_H
