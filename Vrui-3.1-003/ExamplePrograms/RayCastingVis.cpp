#include "RayCastingVis.h"

#include <fstream>
#include <errno.h>
#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBDepthTexture.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBShadow.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTTexture3D.h>
#include <GL/Extensions/GLARBTextureFloat.h>
#include <GL/GLShader.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>
#include <Vrui/DisplayState.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>

/************************************
Methods of class Raycaster::DataItem:
************************************/

RayCastingVis::DataItem::DataItem(void)
    :hasNPOTDTextures(GLARBTextureNonPowerOfTwo::isSupported()),
     depthTextureID(0),depthFramebufferID(0),
     haveFloatTextures(GLARBTextureFloat::isSupported()),
     volumeTextureID(0),volumeTextureVersion(0),colorMapTextureID(0),
     volumeSamplerLoc(-1),colorMapSamplerLoc(-1),
     mcScaleLoc(-1),mcOffsetLoc(-1),
     depthSamplerLoc(-1),depthMatrixLoc(-1),depthSizeLoc(-1),
     eyePositionLoc(-1),stepSizeLoc(-1)
    {
    //Debug Report
     std::cout<<"construct DataItem "<<std::endl;
    /* Check for the required OpenGL extensions: */
    if(!GLShader::isSupported())
        Misc::throwStdErr("GPURaycasting::initContext: Shader objects not supported by local OpenGL");
    //if(!GLARBMultitexture::isSupported()||!GLEXTTexture3D::isSupported())
    //	Misc::throwStdErr("GPURaycasting::initContext: Multitexture or 3D texture extension not supported by local OpenGL");
    if(!GLEXTFramebufferObject::isSupported()||!GLARBDepthTexture::isSupported()||!GLARBShadow::isSupported())
        Misc::throwStdErr("GPURaycasting::initContext: Framebuffer object extension or depth/shadow texture extension not supported by local OpenGL");

    /* Initialize all required OpenGL extensions: */
    GLARBDepthTexture::initExtension();
    GLARBMultitexture::initExtension();
    GLARBShadow::initExtension();
    if(hasNPOTDTextures)
        GLARBTextureNonPowerOfTwo::initExtension();
    if(haveFloatTextures)
        GLARBTextureFloat::initExtension();
    GLEXTFramebufferObject::initExtension();
    GLEXTTexture3D::initExtension();

    /* Create the volume texture object: */
    glGenTextures(1,&volumeTextureID);

    /* Create the color map texture object: */
    glGenTextures(1,&colorMapTextureID);

    /* Create the depth texture: */
    glGenTextures(1,&depthTextureID);
    glBindTexture(GL_TEXTURE_2D,depthTextureID);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,1,1,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
    depthTextureSize[0]=depthTextureSize[1]=1;
    glBindTexture(GL_TEXTURE_2D,0);

    /* Create the depth framebuffer and attach the depth texture to it: */
    glGenFramebuffersEXT(1,&depthFramebufferID);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,depthFramebufferID);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,depthTextureID,0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
    }

RayCastingVis::DataItem::~DataItem(void)
    {
    /* Destroy the depth texture and framebuffer: */
    glDeleteFramebuffersEXT(1,&depthFramebufferID);
    glDeleteTextures(1,&depthTextureID);

    /* Destroy the volume texture object: */
    glDeleteTextures(1,&volumeTextureID);

    /* Destroy the color map texture object: */
    glDeleteTextures(1,&colorMapTextureID);
    }

void RayCastingVis::DataItem::initDepthBuffer(const int* windowSize)
    {
    //Debug Report
//     std::cout<<"Initial DataItem DepthBuffer "<<std::endl;
    /* Calculate the new depth texture size: */
    GLsizei newDepthTextureSize[2];
    if(hasNPOTDTextures)
        {
        /* Use the viewport size: */
        for(int i=0;i<2;++i)
            newDepthTextureSize[i]=windowSize[i];
        }
    else
        {
        /* Pad the viewport size to the next power of two: */
        for(int i=0;i<2;++i)
            for(newDepthTextureSize[i]=1;newDepthTextureSize[i]<windowSize[i];newDepthTextureSize[i]<<=1)
                ;
        }

    /* Bind the depth texture: */
    glBindTexture(GL_TEXTURE_2D,depthTextureID);

    /* Check if the depth texture size needs to change: */
    if(newDepthTextureSize[0]!=depthTextureSize[0]||newDepthTextureSize[1]!=depthTextureSize[1])
        {
        /* Re-allocate the depth texture: */
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,newDepthTextureSize[0],newDepthTextureSize[1],0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);

        /* Store the new depth texture size: */
        for(int i=0;i<2;++i)
            depthTextureSize[i]=newDepthTextureSize[i];
        }

    /* Query the current viewport: */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);

    /* Copy the current depth buffer into the depth texture: */
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,viewport[0],viewport[1],viewport[0],viewport[1],viewport[2],viewport[3]);

    /* Unbind the depth texture: */
    glBindTexture(GL_TEXTURE_2D,0);
    }

/**************************
Methods of class Raycaster:
**************************/

GLMotif::PopupMenu* RayCastingVis::createMainMenu(void)
{
    GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
    mainMenuPopup->setTitle("RayCasting Visualizer");

    GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);

    showPaletteEditorToggle=new GLMotif::ToggleButton("ShowPaletteEditorToggle",mainMenu,"Show Transfer Function Editor");
    showPaletteEditorToggle->getValueChangedCallbacks().add(this,&RayCastingVis::showPaletteEditorCallback);

    mainMenu->manageChild();

    return mainMenuPopup;
}

void RayCastingVis::initDataItem(RayCastingVis::DataItem* dataItem) const
    {
    //Debug Report
     std::cout<<"Initial DataItem"<<std::endl;
    /* Calculate the appropriate volume texture's size: */
    if(dataItem->hasNPOTDTextures)
        {
        /* Use the data size directly: */
        for(int i=0;i<3;++i)
            dataItem->textureSize[i]=dataSize[i];
        }
    else
        {
        /* Pad to the next power of two: */
        for(int i=0;i<3;++i)
            for(dataItem->textureSize[i]=1;dataItem->textureSize[i]<GLsizei(dataSize[i]);dataItem->textureSize[i]<<=1)
                ;
        }

    /* Calculate the texture coordinate box for trilinear interpolation and the transformation from model space to data space: */
    Point tcMin,tcMax;
    for(int i=0;i<3;++i)
        {
        tcMin[i]=Scalar(0.5)/Scalar(dataItem->textureSize[i]);
        tcMax[i]=(Scalar(dataSize[i])-Scalar(0.5))/Scalar(dataItem->textureSize[i]);
        Scalar scale=(tcMax[i]-tcMin[i])/domain.getSize(i);
        dataItem->mcScale[i]=GLfloat(scale);
        dataItem->mcOffset[i]=GLfloat(tcMin[i]-domain.min[i]*scale);
        }
    dataItem->texCoords=Box(tcMin,tcMax);

    /* Create the data volume texture: */
    glBindTexture(GL_TEXTURE_3D,dataItem->volumeTextureID);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP);
    glTexImage3DEXT(GL_TEXTURE_3D,0,GL_INTENSITY,dataItem->textureSize[0],dataItem->textureSize[1],dataItem->textureSize[2],0,GL_LUMINANCE,GL_UNSIGNED_BYTE,0);
    glBindTexture(GL_TEXTURE_3D,0);

    /* Create the color map texture: */
    glBindTexture(GL_TEXTURE_1D,dataItem->colorMapTextureID);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_1D,0);
    }

void RayCastingVis::initShader(RayCastingVis::DataItem* dataItem) const
    {
    //Debug Report
     std::cout<<"Initial Shader "<<std::endl;
    /* Get the shader's uniform locations: */
    dataItem->mcScaleLoc=dataItem->shader.getUniformLocation("mcScale");
    dataItem->mcOffsetLoc=dataItem->shader.getUniformLocation("mcOffset");

    dataItem->depthSamplerLoc=dataItem->shader.getUniformLocation("depthSampler");
    dataItem->depthMatrixLoc=dataItem->shader.getUniformLocation("depthMatrix");
    dataItem->depthSizeLoc=dataItem->shader.getUniformLocation("depthSize");

    dataItem->eyePositionLoc=dataItem->shader.getUniformLocation("eyePosition");
    dataItem->stepSizeLoc=dataItem->shader.getUniformLocation("stepSize");

    /* Get the shader's uniform locations: */
    dataItem->volumeSamplerLoc=dataItem->shader.getUniformLocation("volumeSampler");
    dataItem->colorMapSamplerLoc=dataItem->shader.getUniformLocation("colorMapSampler");

    }

void RayCastingVis::bindShader(const RayCastingVis::PTransform& pmv,const RayCastingVis::PTransform& mv,RayCastingVis::DataItem* dataItem) const
    {
    //Debug Report
//     std::cout<<"Bind shader "<<std::endl;
    /* Set up the data space transformation: */
    glUniform3fvARB(dataItem->mcScaleLoc,1,dataItem->mcScale);
    glUniform3fvARB(dataItem->mcOffsetLoc,1,dataItem->mcOffset);

    /* Bind the ray termination texture: */
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D,dataItem->depthTextureID);
    glUniform1iARB(dataItem->depthSamplerLoc,0);

    /* Set the termination matrix: */
    glUniformMatrix4fvARB(dataItem->depthMatrixLoc,1,GL_TRUE,pmv.getMatrix().getEntries());

    /* Set the depth texture size: */
    glUniform2fARB(dataItem->depthSizeLoc,float(dataItem->depthTextureSize[0]),float(dataItem->depthTextureSize[1]));

    /* Calculate the eye position in model coordinates: */
    Point eye=pmv.inverseTransform(PTransform::HVector(0,0,1,0)).toPoint();
    glUniform3fvARB(dataItem->eyePositionLoc,1,eye.getComponents());

    /* Set the sampling step size: */
    glUniform1fARB(dataItem->stepSizeLoc,stepSize*cellSize);

    /* Bind the volume texture: */
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_3D,dataItem->volumeTextureID);
    glUniform1iARB(dataItem->volumeSamplerLoc,1);

    //Debug Report
//     std::cout<<"Update Volume Texture"<<std::endl;
    /* Check if the volume texture needs to be updated: */
    if(dataItem->volumeTextureVersion!=dataVersion)
        {
        /* Upload the new volume data: */
        //Debug Report
         std::cout<<"Update volume texture version"<<dataVersion<<std::endl;
          std::cout<<"datasize: "<<dataSize[0]<<" "<< dataItem->textureSize[0]<<std::endl;
         glTexImage3DEXT(GL_TEXTURE_3D,0,GL_INTENSITY,dataItem->textureSize[0],dataItem->textureSize[1],
                  dataItem->textureSize[2],0,GL_LUMINANCE,GL_UNSIGNED_BYTE,volumeData.data());

        /* Mark the volume texture as up-to-date: */
        dataItem->volumeTextureVersion=dataVersion;
        }

    //Debug Report
//     std::cout<<"Update ColorMap Texture"<<std::endl;
    /* Bind the color map texture: */
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_1D,dataItem->colorMapTextureID);
    glUniform1iARB(dataItem->colorMapSamplerLoc,2);

    /* Create the stepsize-adjusted colormap with pre-multiplied alpha: */
    GLColorMap* adjustedColorMap = new GLColorMap(GLColorMap::RAINBOW|GLColorMap::RAMP_ALPHA,1.0f,1.0f,1.0,100.0);
    adjustedColorMap->changeTransparency(stepSize*transparencyGamma);
    adjustedColorMap->premultiplyAlpha();

    glTexImage1D(GL_TEXTURE_1D,0,dataItem->haveFloatTextures?GL_RGBA32F_ARB:GL_RGBA,256,0,GL_RGBA,GL_FLOAT,adjustedColorMap->getColors());
    //Debug Report
//     std::cout<<"Finish Update Texture"<<std::endl;
    }

void RayCastingVis::unbindShader(RayCastingVis::DataItem* dataItem) const
    {
    //Debug Report
//     std::cout<<"Unbind Shader"<<std::endl;
    /* Unbind the color map texture: */
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_1D,0);

    /* Bind the volume texture: */
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_3D,0);

    /* Unbind the ray termination texture: */
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D,0);
    }

Polyhedron<RayCastingVis::Scalar>* RayCastingVis::clipDomain(const RayCastingVis::PTransform& pmv,const RayCastingVis::PTransform& mv) const
    {
    /* Clip the render domain against the view frustum's front plane: */
    Point fv0=pmv.inverseTransform(Point(-1,-1,-1));
    Point fv1=pmv.inverseTransform(Point( 1,-1,-1));
    Point fv2=pmv.inverseTransform(Point(-1, 1,-1));
    Point fv3=pmv.inverseTransform(Point( 1, 1,-1));
    Plane::Vector normal=Geometry::cross(fv1-fv0,fv2-fv0)
                         +Geometry::cross(fv3-fv1,fv0-fv1)
                         +Geometry::cross(fv2-fv3,fv1-fv3)
                         +Geometry::cross(fv0-fv2,fv3-fv2);
    Scalar offset=(normal*fv0+normal*fv1+normal*fv2+normal*fv3)*Scalar(0.25);
    Polyhedron<Scalar>* clippedDomain=renderDomain.clip(Plane(normal,offset));

    /* Clip the render domain against all active clipping planes: */
    GLint numClipPlanes;
    glGetIntegerv(GL_MAX_CLIP_PLANES,&numClipPlanes);
    for(GLint i=0;i<numClipPlanes;++i)
        if(glIsEnabled(GL_CLIP_PLANE0+i))
            {
            /* Get the clipping plane's plane equation in eye coordinates: */
            GLdouble planeEq[4];
            glGetClipPlane(GL_CLIP_PLANE0+i,planeEq);

            /* Transform the clipping plane to model coordinates: */
            Geometry::ComponentArray<Scalar,4> hn;
            for(int i=0;i<3;++i)
                hn[i]=Scalar(-planeEq[i]);
            hn[3]=-planeEq[3];
            hn=mv.getMatrix().transposeMultiply(hn);

            /* Clip the domain: */
            Polyhedron<Scalar>* newClippedDomain=clippedDomain->clip(Polyhedron<Scalar>::Plane(Polyhedron<Scalar>::Plane::Vector(hn.getComponents()),-hn[3]-Scalar(1.0e-4)));
            delete clippedDomain;
            clippedDomain=newClippedDomain;
            }

    return clippedDomain;
    }

RayCastingVis::RayCastingVis(int& argc, char**& argv)
    :Vrui::Application(argc,argv),
     domainExtent(0),cellSize(0),stepSize(1),
     mainMenu(0),transFuncEditor(0)
    {
    //initial interface
    mainMenu = createMainMenu();
    Vrui::setMainMenu(mainMenu);
    transFuncEditor=new PaletteEditor;

    //Debug Report
     std::cout<<"Construct RayCastingVis"<<std::endl;
    const unsigned int sDataSize[3] = {256,256,256};
    domain = Box(Point(-1,-1,-1),Point(1,1,1));
    renderDomain = Polyhedron<Scalar>(Polyhedron<Scalar>::Point(domain.min),Polyhedron<Scalar>::Point(domain.max));

    /* Copy the data sizes and calculate the data strides and cell size: */
    ptrdiff_t stride=1;
    for(int i=0;i<3;++i)
        {
        dataSize[i]=sDataSize[i];
        dataStrides[i]=stride;
        stride*=ptrdiff_t(dataSize[i]);
        domainExtent+=Math::sqr(domain.max[i]-domain.min[i]);
        cellSize+=Math::sqr((domain.max[i]-domain.min[i])/Scalar(dataSize[i]-1));
        }
    domainExtent=Math::sqrt(domainExtent);
    cellSize=Math::sqrt(cellSize);

    // initial data
    const char* datasetName = "bin/data/BostonTeapot.raw";
    int volumesize = dataSize[0]*dataSize[1]*dataSize[2];
    /*load sample data*/
    volumeData.resize(volumesize);
    std::ifstream ifs(datasetName, std::ios::binary);

    std::cout<<"open dataset file "<<std::endl;
    if(!ifs.is_open())
      {
        /* fail to open dataset file */
        Misc::throwStdErr("fail to open dataset file:");
        return;
      }
    ifs.read(reinterpret_cast<char *>(&volumeData.front()), volumesize);
    ifs.close();

    //data = new Voxel[dataSize[0]*dataSize[1]*dataSize[2]];
    dataVersion = 1;
    colorMap = 0;
    transparencyGamma = 1.0f;
    }

RayCastingVis::~RayCastingVis(void)
    {
//    delete[] data;
    }

void RayCastingVis::setStepSize(RayCastingVis::Scalar newStepSize)
    {
    /* Set the new step size: */
    stepSize=newStepSize;
    }

void RayCastingVis::initContext(GLContextData& contextData) const
    {
    //Debug Report
     std::cout<<"Initial Context Data "<<std::endl;
    /* Create a new data item: */
    DataItem* dataItem=new DataItem;
    contextData.addDataItem(this,dataItem);

    /* Initialize the data item: */
    initDataItem(dataItem);
    //Debug Report
     std::cout<<"initial and compile shader "<<std::endl;
    try
        {
        /* Load and compile the vertex program: */
        std::string vertexShaderName="bin/Shaders/SingleChannelRaycaster.vs";
        dataItem->shader.compileVertexShader(vertexShaderName.c_str());
        std::string fragmentShaderName="bin/Shaders/SingleChannelRaycaster.fs";
        dataItem->shader.compileFragmentShader(fragmentShaderName.c_str());
        dataItem->shader.linkShader();

        /* Initialize the raycasting shader: */
        initShader(dataItem);
        }
    catch(std::runtime_error err)
        {
        /* Print an error message, but continue: */
        std::cerr<<"SingleChannelRaycaster::initContext: Caught exception "<<err.what()<<std::endl;
        }
    }

void RayCastingVis::glRenderAction(GLContextData& contextData) const
    {
    //Debug Report
//     std::cout<<"Start gl render action "<<std::endl;
    /* Get the OpenGL-dependent application data from the GLContextData object: */
    DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);

    /* Bail out if shader is invalid: */
    if(!dataItem->shader.isValid())
        return;

    /* Save OpenGL state: */
    glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_POLYGON_BIT);

    /* Initialize the ray termination depth frame buffer: */
    const Vrui::DisplayState& vds=Vrui::getDisplayState(contextData);
    dataItem->initDepthBuffer(vds.window->getWindowSize());

    /* Bind the ray termination framebuffer: */
    GLint currentFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&currentFramebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,dataItem->depthFramebufferID);

    /* Get the projection and modelview matrices: */
    PTransform mv=glGetModelviewMatrix<Scalar>();
    PTransform pmv=glGetProjectionMatrix<Scalar>();
    pmv*=mv;

    /* Clip the render domain against the view frustum's front plane and all clipping planes: */
    Polyhedron<Scalar>* clippedDomain=clipDomain(pmv,mv);

    /* Draw the clipped domain's back faces to the depth buffer as ray termination conditions: */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    clippedDomain->drawFaces();

    /* Unbind the depth framebuffer: */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,currentFramebuffer);

    /* Install the GLSL shader program: */
    dataItem->shader.useProgram();
    bindShader(pmv,mv,dataItem);

    /* Draw the clipped domain's front faces: */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glCullFace(GL_BACK);
    clippedDomain->drawFaces();

    /* Uninstall the GLSL shader program: */
    unbindShader(dataItem);
    GLShader::disablePrograms();

    /* Clean up: */
    delete clippedDomain;

    /* Restore OpenGL state: */
    glPopAttrib();
    }
void RayCastingVis::updateData(void)
    {
    /* Bump up the data version number: */
    ++dataVersion;
    }

void RayCastingVis::setColorMap(const GLColorMap* newColorMap)
    {
    colorMap=newColorMap;
    }

void RayCastingVis::setTransparencyGamma(GLfloat newTransparencyGamma)
    {
    transparencyGamma=newTransparencyGamma;
    }

void RayCastingVis::frame()
{

}

void RayCastingVis::display(GLContextData &contextData) const
{
    //Debug Report
//     std::cout<<"Start Display"<<std::endl;
    glRenderAction(contextData);
}

void RayCastingVis::TransferFuncEditorCallback(Misc::CallbackData *cbData)
{

}
void RayCastingVis::showPaletteEditorCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    /* Hide or show palette editor based on toggle button state: */
    if(cbData->set)
        Vrui::popupPrimaryWidget(transFuncEditor);
    else
        Vrui::popdownPrimaryWidget(transFuncEditor);
}

VRUI_APPLICATION_RUN(RayCastingVis)
