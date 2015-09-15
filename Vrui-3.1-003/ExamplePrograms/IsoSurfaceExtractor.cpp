#include "IsoSurfaceExtractor.h"

namespace visualization {


IsoSurfaceExtractor::IsoSurfaceExtractor( DataSetGrid* sDataSet)
    :extractionmode(FLAT),dataset(sDataSet),isosurface(0),
      isoValue(0)
{

}

IsoSurfaceExtractor::~IsoSurfaceExtractor()
{

}

void IsoSurfaceExtractor::ExtractIsoSurface(const Scalar newIsoValue, Isosurface* newIsoSurface)
{
    if(newIsoSurface == NULL)
    {
        cout<<"triangle set is null"<<endl;
        return;
    }
    isosurface = newIsoSurface;
    isoValue = newIsoValue;

    size_t numCells = dataset->getTotalNumCells();
    DataSetGrid::cellitr cIt = dataset->beginCells();

    std::cout<<"total cell Num: "<< numCells<<std::endl;

    size_t cellIndex=0;
    if(extractionmode==FLAT)
        {
        for(int percent=1;percent<=100;++percent)
            {
            size_t cellIndexEnd=(numCells*percent)/100;
            for(;cellIndex<cellIndexEnd;++cellIndex,++cIt)
                {
                /* Extract the cell's isosurface fragment: */
//                std::cout<<"Flat extraction: cellid"<<cellIndex<<" percents: "<<percent<<std::endl;
                ExtractFlatIsoSurface(*cIt);
                }

//			/* Update the busy dialog: */
//			algorithm->callBusyFunction(float(percent));
            }
        }
    else
        {
        for(int percent=1;percent<=100;++percent)
            {
            size_t cellIndexEnd=(numCells*percent)/100;
            for(;cellIndex<cellIndexEnd;++cellIndex,++cIt)
                {
                /* Extract the cell's isosurface fragment: */
                std::cout<<"Smooth extraction cell:"<<std::endl;
                ExtractSmoothIsoSurface(*cIt);
                }

//			/* Update the busy dialog: */
//			algorithm->callBusyFunction(float(percent));
            }
        }
    isosurface->flush();

//    cout<<"isosurface vertices num"<<isosurface->getNumVertices()<<endl;

    /* Clean up: */
    isosurface=0;
//    vertexIndices.clear();
}

void IsoSurfaceExtractor::ExtractFlatIsoSurface(DataSetCell& cell)
{
    int caseindex = 0x0;
    for(int i=0; i<cell.getVerticesNum(); i++)
    {
        if(cell.getVerticesValue(i) >= isoValue)
            caseindex |=1<<i;
    }
    /* Calculate the edge intersection points: */
    Point edgeVertices[DataSetCell::edgeNum];
    int cem=CaseTable::edgeMasks[caseindex];
//    std::cout<<"isovalue: "<<isoValue<<" cell case index: "<<hex<<caseindex<<" cell edgeMask:"<<cem<<std::endl;
    for(int edge=0;edge<DataSetCell::edgeNum;++edge)
    {
//        cout<<"edge "<<edge<<" end: value: "<<cell.getEdgeEndPointValue(edge,0)<<" "<<cell.getEdgeEndPointValue(edge,1)<<endl;
        if(cem&(1<<edge))
            {
            /* Calculate intersection point on the edge: */
            float vi0 = cell.getEdgeEndPointValue(edge,0);
            float vi1 = cell.getEdgeEndPointValue(edge,1);
//            cout<<"endPoint value: "<<vi0<<" "<<vi1<<endl;
            float w1=(isoValue-vi0)/(vi1-vi0);
            edgeVertices[edge]=cell.calEdgePointPosition(edge,w1);
//            std::cout<<"edge num:"<<edge<<" value position:"<<w1<<" point:"<<edgeVertices[edge][0]<<" "<<edgeVertices[edge][1]<<" "<<edgeVertices[edge][2]<<std::endl;
            }
    }
    /* Store the resulting fragment in the isosurface: */
    for(const int* ctei=CaseTable::triangleEdgeIndices[caseindex];*ctei>=0;ctei+=3)
        {
        Index* iPtr=isosurface->getNextTriangle();
        Vector normal=Geometry::cross(edgeVertices[ctei[1]]-edgeVertices[ctei[0]],edgeVertices[ctei[2]]-edgeVertices[ctei[0]]);
        for(int i=0;i<3;++i)
            {
            Vertex* vertex=isosurface->getNextVertex();
            vertex->normal=normal.getComponents();
            vertex->position=edgeVertices[ctei[i]].getComponents();
            iPtr[i]=isosurface->addVertex();
            }
        isosurface->addTriangle();
        }
}

void IsoSurfaceExtractor::ExtractSmoothIsoSurface( DataSetCell &cell)
{

}

    }
