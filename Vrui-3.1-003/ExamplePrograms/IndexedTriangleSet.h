
#ifndef VISUALIZATION_TEMPLATIZED_INDEXEDTRIANGLESET_INCLUDED
#define VISUALIZATION_TEMPLATIZED_INDEXEDTRIANGLESET_INCLUDED

#include "DataSetGrid.h"

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLVertex.h>



/* Forward declarations: */
namespace Cluster {
class MulticastPipe;
}

namespace visualization {
class DataSetGrid;
}

namespace visualization {

//namespace Templatized {

class IndexedTriangleSet:public GLObject
    {
    /* Embedded classes: */
    public:
    typedef DataSetGrid::Scalar Scalar;
    typedef GLVertex<void,0,void,0,Scalar,Scalar,3> Vertex; // Type for triangle vertices
    typedef GLuint Index; // Type for vertex indices

    private:
    static const size_t vertexChunkSize=10000; // Number of vertices per vertex chunk
    static const size_t indexChunkSize=3333; // Number of triangles per index chunk

    struct VertexChunk // Structure for vertex buffer chunks
        {
        /* Elements: */
        public:
        VertexChunk* succ; // Pointer to next vertex buffer chunk
        Vertex vertices[vertexChunkSize]; // Array of vertices

        /* Constructors and destructors: */
        VertexChunk(void)
            :succ(0)
            {
            }
        };

    struct IndexChunk // Structure for index buffer chunks
        {
        /* Elements: */
        public:
        IndexChunk* succ; // Pointer to next index buffer chunk
        Index indices[indexChunkSize*3]; // Array of vertex indices

        /* Constructors and destructors: */
        IndexChunk(void)
            :succ(0)
            {
            }
        };

    struct DataItem:public GLObject::DataItem
        {
        /* Elements: */
        public:
        GLuint vertexBufferId; // ID of buffer object for vertex data
        GLuint indexBufferId; // ID of buffer object for index data
        unsigned int version; // Version number of the triangle set in the buffer objects
        size_t numVertices; // Number of vertices in the vertex buffer
        size_t numTriangles; // Number of triangles (index triples) in the index buffer

        /* Constructors and destructors: */
        DataItem(void);
        virtual ~DataItem(void);
        };

    /* Elements: */
    private:
    Cluster::MulticastPipe* pipe; // Pipe to stream triangle set data in a cluster environment (owned by caller)
    unsigned int version; // Version number of the triangle set (incremented on each clear operation)
    size_t numVertices; // Number of vertices in the triangle set
    size_t numTriangles; // Number of triangles (index triples) in the triangle set
    VertexChunk* vertexHead; // Pointer to first vertex buffer chunk
    VertexChunk* vertexTail; // Pointer to last vertex buffer chunk
    IndexChunk* indexHead; // Pointer to first index buffer chunk
    IndexChunk* indexTail; // Pointer to last index buffer chunk
    size_t tailNumSentVertices; // Number of vertices in the last vertex buffer chunk that were already sent across the pipe
    size_t tailNumSentTriangles; // Number of triangles (index triples) in the last index buffer chunk that were already sent across the pipe
    size_t numVerticesLeft; // Number of vertices left in last vertex buffer chunk
    size_t numTrianglesLeft; // Number of triangles (index triples) left in last index buffer chunk
    Vertex* nextVertex; // Pointer to next vertex to be stored
    Index* nextTriangle; // Pointer to next triangle (index triple) to be stored

    /* Private methods: */
    void addNewVertexChunk(void); // Adds a new chunk to the vertex buffer
    void addNewIndexChunk(void); // Adds a new chunk to the index buffer

    /* Constructors and destructors: */
    public:
    IndexedTriangleSet(Cluster::MulticastPipe* sPipe); // Creates empty triangle set for given multicast pipe (or 0 in single-machine environment)
    private:
    IndexedTriangleSet(const IndexedTriangleSet& source); // Prohibit copy constructor
    IndexedTriangleSet& operator=(const IndexedTriangleSet& source); // Prohibit assignment operator
    public:
    virtual ~IndexedTriangleSet(void); // Destroys triangle set

    /* Methods: */
    virtual void initContext(GLContextData& contextData) const;
    void clear(void); // Removes all triangles from the set
    Vertex* getNextVertex(void) // Returns pointer to next vertex in buffer
        {
        /* Check if there is room in the last vertex buffer chunk to add another vertex: */
        if(numVerticesLeft==0)
            addNewVertexChunk();

        /* Return pointer to the next vertex: */
        return nextVertex;
        }
    Index addVertex(void) // Just advances the vertex counter and returns the most recent index; assumes caller wrote data into buffer
        {
        /* Increment the vertex count: */
        ++numVertices;
        --numVerticesLeft;
        ++nextVertex;

        /* Return the last vertex' index: */
        return Index(numVertices-1);
        }
    Index* getNextTriangle(void) // Returns pointer to next index triple in buffer
        {
        /* Check if there is room in the last index buffer chunk to add another index triple: */
        if(numTrianglesLeft==0)
            addNewIndexChunk();

        /* Return pointer to next index triple: */
        return nextTriangle;
        }
    void addTriangle(void) // Just advances the triangle counter; assumes caller wrote data into buffer
        {
        /* Increment the triangle count: */
        ++numTriangles;
        --numTrianglesLeft;
        nextTriangle+=3;
        }
    void receive(void); // Receives triangle set data via multicast pipe until next flush() point
    void flush(void); // Sends pending triangle set data across the multicast pipe and terminates receive() method on slaves
    size_t getNumVertices(void) const // Returns number of vertices currently in buffer
        {
        return numVertices;
        }
    size_t getNumTriangles(void) const // Returns number of triangles currently in buffer
        {
        return numTriangles;
        }
    void glRenderAction(GLContextData& contextData) const; // Renders all triangles in the buffer
    };

}

//}

#endif
