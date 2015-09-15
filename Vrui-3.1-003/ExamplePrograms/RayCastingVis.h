#ifndef RAYCASTINGVIS_H
#define RAYCASTINGVIS_H

#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>
#include <GL/GLColorMap.h>
//#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Application.h>
#include <GLMotif/Menu.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Slider.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/ListBox.h>

#include "Polyhedron.h"
#include "PaletteEditor.h"
#include "pointcloudvis.h"
#include "DataSetGrid.h"
#include "DataFilter.h"
#include "PointCloudGrid.h"

/* Forward declarations: */
namespace GLMotif {
class Widget;
class Popup;
class PopupMenu;
class PopupWindow;
class RowColumn;
class TextField;
class ToggleButton;
}
namespace Misc {
class CallbackData;
}

namespace visualization {
class PointCloudVis;
class DataSetGrid;
class IsoSurfaceExtractor;
class IndexedTriangleSet;
class DataFilter;
}
class RayCastingVis:public Vrui::Application,public GLObject
    {
    /* Embedded classes: */
    public:
    typedef float Scalar;
    typedef Geometry::Point<Scalar,2> Point2D;
    typedef Geometry::Point<Scalar,3> Point;
    typedef Geometry::Box<Scalar,3> Box;
    typedef Geometry::Plane<Scalar,3> Plane;
    typedef Geometry::ProjectiveTransformation<Scalar,3> PTransform;
    typedef GLubyte Voxel; // Type for voxel data
    typedef visualization::PointCloudVis PointDataSet;
    typedef visualization::IsoSurfaceExtractor ISExtrctor;
    typedef visualization::DataSetGrid DataSetGrid;
    typedef visualization::DataFilter DataFilter;
    typedef visualization::IndexedTriangleSet Surface;
    typedef visualization::PointCloudGrid PointDataGrid;

    protected:
    struct DataItem:public GLObject::DataItem // Structure containing per-context state
        {
        /* Elements: */
        public:
        bool hasNPOTDTextures; // Flag whether the local OpenGL supports non-power of two textures

        GLsizei textureSize[3]; // Size of textures able to hold the volume data
        Box texCoords; // Domain of texture coordinates to access the volume data
        GLfloat mcScale[3],mcOffset[3]; // Scale factors and offsets from model space to data space

        GLuint depthTextureID; // Texture object ID of the depth texture used for ray termination
        GLuint depthFramebufferID; // Framebuffer object ID to render to the ray termination buffer
        GLsizei depthTextureSize[2]; // Current size of the depth texture

        int transFuncTexLoc;
        int deltastepLoc;
        int OCSizeLoc;
        GLuint vertexBufferObjectID;
        GLuint vertexArrayObjectID;

        GLShader shader; // Shader object for the raycasting algorithm

        bool usePreIntegration; // Flag whether use pre integeted transfer function
        GLuint preIntTextureID;
        GLuint preIntFramebufferID;
        GLenum colAtt;
        GLShader preIntShader;

        bool haveFloatTextures; // Flag whether the local OpenGL supports floating-point textures

        GLuint volumeTextureID; // Texture object ID for volume data texture
        unsigned int volumeTextureVersion; // Version number of volume data texture
        GLuint colorMapTextureID; // Texture object ID for stepsize-adjusted color map texture

        int volumeSamplerLoc; // Location of the volume data texture sampler
        int colorMapSamplerLoc; // Location of the color map texture sampler
        int mcScaleLoc; // Location of the scale factors from model coordinates to data coordinates
        int mcOffsetLoc; // Location of the offset from model coordinates to data coordinates
        int depthSamplerLoc; // Location of the depth texture sampler uniform variable
        int depthMatrixLoc; // Location of the depth texture transformation matrix uniform variable
        int depthSizeLoc; // Location of the depth texture size uniform variable
        int eyePositionLoc; // Location of the eye position uniform variable
        int stepSizeLoc; // Location of the step size uniform variable

        int lightFlagLoc; // Location of the light flag variable

        /* Constructors and destructors: */
        DataItem(void);
        virtual ~DataItem(void);

        /* Methods: */
        virtual void initDepthBuffer(const int* windowSize); // Creates the depth texture and framebuffer based on current OpenGL state and copies the current depth buffer contents
        void initialPreIntBuffer(GLColorMap *colormap);
        void updatePreIntTexture();
        };

    /* Elements: */
    protected:
    std::vector<float> volumeData;
    float* volumeDataPtr;
    PointDataSet* pointVolume;
    PointDataGrid* dataGrids;
    bool gridSizeHasChanged;
    ISExtrctor* iSExtrctor;
    DataSetGrid* dataSetGrid;
    Surface* isosurface;
    string currentElement;
    vector<string> currentElementList;
    int pointCloudSize;
    Voxel* data;
    DataFilter* dataFilter;
    unsigned int dataVersion; // Version number of the volume dataset to track changes
    GLColorMap* colorMap; // Pointer to the color map
    GLfloat transparencyGamma; // Adjustment factor for color map's overall opacity

    unsigned int dataSize[3]; // Size of volume data
    ptrdiff_t dataStrides[3]; // Volume data strides in x, y, z dimensions
    Box domain; // The volume renderer's domain box in model space
    Scalar domainExtent; // Length of longest ray through domain
    Scalar cellSize; // The data set's cell size
    Polyhedron<Scalar> renderDomain; // Polyhedron used to render the clipped data set

    Scalar stepSize; // The ray casting step size in cell size units

    /* Interface Element */
    GLMotif::PopupMenu* mainMenu; // The main menu widget
    GLMotif::ToggleButton* showPaletteEditorToggle; // Toggle button to show the palette editor
    GLMotif::ToggleButton* showRenderSettiingToggle; // Toggle button to show the render setting dialog
    GLMotif::ToggleButton* PhongReflectionToggle; // Toggle button to control phong shading lighting
    GLMotif::ToggleButton* showPointCloudSetteingToggle;

    GLMotif::PopupWindow* RenderSettingDlg;
    GLMotif::PopupWindow* PointCloudsDlg;
    GLMotif::PopupWindow* ElementListDlg;

    GLMotif::ListBox* elementList;
    PaletteEditor* transFuncEditor;

    /* Interface Parameter */
    bool LightFlag;
    float ambinetCoE;
    float specularCoE;
    float diffuseCoE;
//    float stepSize;

    /* Private methods: */
    private:
    GLMotif::PopupMenu* createMainMenu(void);
    GLMotif::PopupWindow* createRenderSettingDlg(void);
    GLMotif::PopupWindow* createPointCloudsSettingDlg(void);
    void UpdateElementList();

    /* Protected methods: */
    protected:
    virtual void initDataItem(DataItem* dataItem) const; // Initializes the given context data item
    virtual void initShader(DataItem* dataItem) const; // Initializes the GLSL raycasting shader
    virtual void bindShader(const PTransform& pmv,const PTransform& mv,DataItem* dataItem) const; // Prepares the GLSL raycasting shader for rendering
    virtual void unbindShader(DataItem* dataItem) const; // Unbinds the GLSL raycasting shader after rendering
    Polyhedron<Scalar>* clipDomain(const PTransform& pmv,const PTransform& mv) const; // Clips the domain against the view frustum and all clipping planes and returns the resulting polyhedron

    /* Constructors and destructors: */
    public:
    RayCastingVis(int& argc, char**& argv); // Creates a raycaster for the given data and domain sizes
    virtual ~RayCastingVis(void); // Destroys the raycaster

    /* New methods: */
    const unsigned int* getDataSize(void) const // Returns the raycaster's data size
        {
        return dataSize;
        }
    unsigned int getDataSize(int dimension) const // Returns one dimension of the raycaster's data size
        {
        return dataSize[dimension];
        }
    const ptrdiff_t* getDataStrides(void) const // Returns the volume data's strides in x, y, z directions
        {
        return dataStrides;
        }
    ptrdiff_t getDataStrides(int dimension) const // Returns one dimension of the volume data's strides
        {
        return dataStrides[dimension];
        }
    const Box& getDomain(void) const // Returns the raycaster's domain box in model space
        {
        return domain;
        }
    Scalar getCellSize(void) const // Returns the data's average cell size
        {
        return cellSize;
        }
    Scalar getStepSize(void) const // Returns the raycaster's step size in cell size units
        {
        return stepSize;
        }
    virtual void setStepSize(Scalar newStepSize); // Sets the raycaster's step size in cell size units
    virtual void glRenderAction(GLContextData& contextData) const; // Renders the data using current settings from the current OpenGL context

    /* Methods from GLObject: */
    virtual void initContext(GLContextData& contextData) const;
    /* Methods from Vrui::Application: */
    virtual void frame(void);
    virtual void display(GLContextData& contextData) const;

    virtual void updateData(void); // Notifies the raycaster that the volume dataset has changed
    void setColorMap(GLColorMap* newColorMap); // Sets the raycaster's color map
    void setTransparencyGamma(GLfloat newTransparencyGamma); // Sets the opacity adjustment factor
    void loadDataSet(char * datafileName, int sizeX, int sizeY, int sizeZ);

    /* Callback Function */
    void TransferFuncEditorCallback(Misc::CallbackData* cbData);
    void menuToggleSelectCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void savePaletteCallback(Misc::CallbackData* cbData);
    void renderDlgSlideChangeCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
    void renderDlgToggleChangeCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void renderDialogCloseCallback(Misc::CallbackData* cbData);
    void loadElementsCallback(Misc::CallbackData*);
    void loadElementsOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    void loadElementsCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData);
    void selectElementCallback(Misc::CallbackData* cbData);
    void clearElementCallback(Misc::CallbackData* cbData);

    /* Pre Integreted transfer function Method*/
    void bindPreIntShader(DataItem* dataItem) const;
    void unbindPreIntShader(DataItem* dataItem)const;
    void drawPreIntTexture(GLContextData& contextData) const;
};

#endif // RAYCASTINGVIS_H
