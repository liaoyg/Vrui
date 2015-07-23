#include "RayCastingVis.h"

#include <fstream>
#include <errno.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/CreateNumberedFileName.h>
#include <Math/Math.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBDepthTexture.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBShadow.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBVertexArrayObject.h>
#include <GL/Extensions/GLEXTTexture3D.h>
#include <GL/Extensions/GLARBTextureFloat.h>
#include <GL/GLShader.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>
#include <Vrui/DisplayState.h>
#include <Vrui/OpenFile.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Menu.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/TextField.h>

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

    if(GLARBVertexBufferObject::isSupported())
        {
        /* Initialize the vertex buffer object extension: */
        GLARBVertexBufferObject::initExtension();

        /* Create a vertex buffer object: */
        glGenBuffersARB(1,&vertexBufferObjectID);
        }
    if(GLARBVertexArrayObject::isSupported())
        {
        /* Initialize the vertex buffer object extension: */
        GLARBVertexArrayObject::initExtension();

        /* Create a vertex buffer object: */
        glGenVertexArrays(1,&vertexArrayObjectID);
        }
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

    if(vertexBufferObjectID!=0)
        {
        /* Destroy the vertex buffer object: */
        glDeleteBuffersARB(1,&vertexBufferObjectID);
        }

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

void RayCastingVis::DataItem::initialPreIntBuffer(GLColorMap *colormap)
{
    /* Create the depth texture: */
    std::cout<<"initial preint buffer"<<std::endl;
    glGenTextures(1,&preIntTextureID);
    glBindTexture(GL_TEXTURE_2D,preIntTextureID);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,256,256,0,GL_RGBA,GL_FLOAT,colormap->getColors());
//    depthTextureSize[0]=depthTextureSize[1]=1;
    glBindTexture(GL_TEXTURE_2D,0);

    /* Create the depth framebuffer and attach the depth texture to it: */
    glGenFramebuffersEXT(1,&preIntFramebufferID);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,preIntFramebufferID);
    colAtt = GL_COLOR_ATTACHMENT0_EXT;
    glDrawBuffer(colAtt);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,colAtt,GL_TEXTURE_2D,preIntTextureID,0);
//    glDrawBuffer(GL_NONE);
//    glReadBuffer(GL_NONE);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

    /* initial shader*/
    deltastepLoc=preIntShader.getUniformLocation("delta_step");
    OCSizeLoc=preIntShader.getUniformLocation("OCSize");

    transFuncTexLoc=preIntShader.getUniformLocation("transferFTex");
}

void RayCastingVis::DataItem::updatePreIntTexture()
{

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
    showPaletteEditorToggle->getValueChangedCallbacks().add(this,&RayCastingVis::menuToggleSelectCallback);

    showRenderSettiingToggle=new GLMotif::ToggleButton("ShowRenderSettingToggle",mainMenu,"Show Render Setting Dialog");
    showRenderSettiingToggle->getValueChangedCallbacks().add(this,&RayCastingVis::menuToggleSelectCallback);

    mainMenu->manageChild();

    return mainMenuPopup;
}
GLMotif::PopupWindow* RayCastingVis::createRenderSettingDlg(void)
{
    const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();

    GLMotif::PopupWindow* renderDialogPopup=new GLMotif::PopupWindow("RenderDialogPopup",Vrui::getWidgetManager(),"Render Settings");
    renderDialogPopup->setResizableFlags(true,false);
    renderDialogPopup->setCloseButton(true);
    renderDialogPopup->getCloseCallbacks().add(this,&RayCastingVis::renderDialogCloseCallback);

    GLMotif::RowColumn* renderDialog=new GLMotif::RowColumn("RenderDialog",renderDialogPopup,false);
    renderDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
    renderDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
    renderDialog->setNumMinorWidgets(2);

    GLMotif::ToggleButton* showSurfaceToggle=new GLMotif::ToggleButton("lightFlag",renderDialog,"Phong Shading");
    showSurfaceToggle->setBorderWidth(0.0f);
    showSurfaceToggle->setMarginWidth(0.0f);
    showSurfaceToggle->setHAlignment(GLFont::Left);
    showSurfaceToggle->setToggle(LightFlag);
    showSurfaceToggle->getValueChangedCallbacks().add(this,&RayCastingVis::renderDlgToggleChangeCallback);

    new GLMotif::Blind("Blind1",renderDialog);

    new GLMotif::Label("AmbientLabel",renderDialog,"AmbientCoE");

    GLMotif::Slider* ambientCoeSlider=new GLMotif::Slider("AmbientSlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
    ambientCoeSlider->setValueRange(0.0,1.0,0.001);
    ambientCoeSlider->setValue(ambinetCoE);
    ambientCoeSlider->getValueChangedCallbacks().add(this,&RayCastingVis::renderDlgSlideChangeCallback);

    new GLMotif::Label("specularLabel",renderDialog,"specularCoE");

    GLMotif::Slider* specularCoeSlider=new GLMotif::Slider("SpecularSlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
    specularCoeSlider->setValueRange(0.0,1.0,0.001);
    specularCoeSlider->setValue(specularCoE);
    specularCoeSlider->getValueChangedCallbacks().add(this,&RayCastingVis::renderDlgSlideChangeCallback);

    new GLMotif::Label("diffuseLabel",renderDialog,"diffuseCoE");

    GLMotif::Slider* diffuseCoeSlider=new GLMotif::Slider("DiffuseSlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
    diffuseCoeSlider->setValueRange(0.0,1.0,0.001);
    diffuseCoeSlider->setValue(diffuseCoE);
    diffuseCoeSlider->getValueChangedCallbacks().add(this,&RayCastingVis::renderDlgSlideChangeCallback);

    new GLMotif::Blind("Blind1",renderDialog);

    new GLMotif::Label("stepSizeLabel",renderDialog,"Step Size");

    GLMotif::Slider* stepSizeSlider=new GLMotif::Slider("StepSizeSlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
    stepSizeSlider->setValueRange(0.5,2.0,0.001);
    stepSizeSlider->setValue(stepSize);
    stepSizeSlider->getValueChangedCallbacks().add(this,&RayCastingVis::renderDlgSlideChangeCallback);

    renderDialog->manageChild();

    return renderDialogPopup;
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

    dataItem->volumeSamplerLoc=dataItem->shader.getUniformLocation("volumeSampler");
    dataItem->colorMapSamplerLoc=dataItem->shader.getUniformLocation("colorMapSampler");

    dataItem->lightFlagLoc=dataItem->shader.getUniformLocation("lightFlag");

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

    /* Set the light shading flag bool value */
    glUniform1iARB(dataItem->lightFlagLoc,LightFlag);

    /* light setting coefficient*/
    glUniform1fARB(dataItem->shader.getUniformLocation("ambientCoE"),ambinetCoE);
    glUniform1fARB(dataItem->shader.getUniformLocation("specularCoE"),specularCoE);
    glUniform1fARB(dataItem->shader.getUniformLocation("diffuseCoE"),diffuseCoE);


    /* Bind the volume texture: */
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_3D,dataItem->volumeTextureID);
    glUniform1iARB(dataItem->volumeSamplerLoc,1);

    /* Check if the volume texture needs to be updated: */
    if(dataItem->volumeTextureVersion!=dataVersion)
        {
        /* Upload the new volume data: */
        //Debug Report
         std::cout<<"Update volume texture version"<<dataVersion<<std::endl;
          std::cout<<"datasize: "<<dataSize[0]<<" "<< dataItem->textureSize[0]<<std::endl;
//         glTexImage3DEXT(GL_TEXTURE_3D,0,GL_INTENSITY,dataItem->textureSize[0],dataItem->textureSize[1],
//                  dataItem->textureSize[2],0,GL_LUMINANCE,GL_UNSIGNED_BYTE,volumeData.data());
          glTexImage3DEXT(GL_TEXTURE_3D,0,GL_INTENSITY,pointCloudSize,pointCloudSize,
                   pointCloudSize,0,GL_LUMINANCE,GL_FLOAT,(GLvoid*)pointVolume->getVolumeDataPtr());

        /* Mark the volume texture as up-to-date: */
        dataItem->volumeTextureVersion=dataVersion;
        }

    /* Bind the color map texture: */
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_1D,dataItem->colorMapTextureID);
    glUniform1iARB(dataItem->colorMapSamplerLoc,2);

    glTexImage1D(GL_TEXTURE_1D,0,dataItem->haveFloatTextures?GL_RGBA32F_ARB:GL_RGBA,256,0,GL_RGBA,GL_FLOAT,colorMap->getColors());
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
     domainExtent(0),cellSize(0),stepSize(2),
     mainMenu(0),transFuncEditor(0)
    {
    //Load datasrc path
    const char* rangeFileName=0;
    const char* dataSrcFileName = 0;
    pointCloudSize = 50;
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
    std::cout<<rangeFileName<<" + "<<dataSrcFileName<<std::endl;
    pointVolume = new PointCloudVis(rangeFileName, dataSrcFileName, pointCloudSize);




    //initial interface
    mainMenu = createMainMenu();
    Vrui::setMainMenu(mainMenu);
    transFuncEditor=new PaletteEditor;
    transFuncEditor->getColorMapChangedCallbacks().add(this,&RayCastingVis::TransferFuncEditorCallback);
//    transFuncEditor->getSavePaletteCallbacks().add(this,&RayCastingVis::savePaletteCallback);
    RenderSettingDlg = createRenderSettingDlg();

    //initial interface parameter
    ambinetCoE = 0.6;
    specularCoE = 0.6;
    diffuseCoE = 0.5;

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

    std::cout<<"open dataset file "<<datasetName<<std::endl;
    if(!ifs.is_open())
      {
        /* fail to open dataset file */
        Misc::throwStdErr("fail to open dataset file:");
        return;
      }
    ifs.read(reinterpret_cast<char *>(&volumeData.front()), volumesize);
    ifs.close();

    std::cout<<"volumedata: "<<volumeData.front()<<std::endl;

    //data = new Voxel[dataSize[0]*dataSize[1]*dataSize[2]];
    dataVersion = 1;
    colorMap = new GLColorMap(GLColorMap::RAINBOW|GLColorMap::RAMP_ALPHA,1.0f,1.0f,1.0,100.0);
    colorMap->changeTransparency(stepSize*transparencyGamma);
    colorMap->premultiplyAlpha();
    transparencyGamma = 1.0f;

    /* interface parameter initialization */
    LightFlag = false;
    }

RayCastingVis::~RayCastingVis(void)
    {
//    delete[] data;
    delete pointVolume;
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
        std::string vertexShaderName="bin/Shaders/SingleChannelRaycaster.vert";
        dataItem->shader.compileVertexShader(vertexShaderName.c_str());
        std::string fragmentShaderName="bin/Shaders/SingleChannelRaycaster.frag";
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

     /* initial and compile pre-integret shader*/
     try
         {
         /* Load and compile the vertex program: */
         std::string vertexShaderName="bin/Shaders/PreIntegration.vert";
         dataItem->preIntShader.compileVertexShader(vertexShaderName.c_str());
         std::string fragmentShaderName="bin/Shaders/PreIntegration.frag";
         dataItem->preIntShader.compileFragmentShader(fragmentShaderName.c_str());
         dataItem->preIntShader.linkShader();

         /* Initialize the raycasting shader: */
         dataItem->initialPreIntBuffer(colorMap);
         }
     catch(std::runtime_error err)
         {
         /* Print an error message, but continue: */
         std::cerr<<"SingleChannelRaycaster::initContext: Caught exception preintegration shader "<<err.what()<<std::endl;
         }
     /* Check if the vertex buffer object extension is supported: */

//     if(dataItem->vertexBufferObjectID>0)
//         {
//         std::vector<Point2D> preIntVertices;
//         preIntVertices.push_back(Point2D(-1.0,-1.0));
//         preIntVertices.push_back(Point2D(-1.0,1.0));
//         preIntVertices.push_back(Point2D(1.0,1.0));
//         preIntVertices.push_back(Point2D(1.0,-1.0));
//         /* Create a vertex buffer object to store the points' coordinates: */
//         glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectID);
//         glBufferDataARB(GL_ARRAY_BUFFER_ARB,preIntVertices.size()*sizeof(Point2D),0,GL_STATIC_DRAW_ARB);

//         /* Protect the vertex buffer object: */
//         glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
//         }
    }

void DrawTriangle()
{
    glBegin(GL_TRIANGLES);
        glVertex2f(0.0f,1.0f);
        glVertex2f(-1.0f,-1.0f);
        glVertex2f(1.0f,-1.0f);
    glEnd();
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

    //test draw pre-intgretion texture
//    drawPreIntTexture(contextData);

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

void RayCastingVis::setColorMap(GLColorMap* newColorMap)
    {
    colorMap=newColorMap;
    }

void RayCastingVis::setTransparencyGamma(GLfloat newTransparencyGamma)
    {
    transparencyGamma=newTransparencyGamma;
    }

void RayCastingVis::loadDataSet(char *datafileName, int sizeX, int sizeY, int sizeZ)
{
    // initial data
//    const char* datasetName = "bin/data/BostonTeapot.raw";
    int volumesize = sizeX*sizeY*sizeZ;
    /*load sample data*/
    volumeData.resize(volumesize);
    std::ifstream ifs(datafileName, std::ios::binary);

    std::cout<<"open dataset file: "<<datafileName<<std::endl;
    if(!ifs.is_open())
      {
        /* fail to open dataset file */
        Misc::throwStdErr("fail to open dataset file:");
        return;
      }
    ifs.read(reinterpret_cast<char *>(&volumeData.front()), volumesize);
    ifs.close();
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
    transFuncEditor->exportColorMap(*colorMap);
    //Debug Report
     std::cout<<"export ColorMap"<<std::endl;
    Vrui::requestUpdate();
}
void RayCastingVis::menuToggleSelectCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    /* Hide or show palette editor based on toggle button state: */
    if(strcmp(cbData->toggle->getName(),"ShowPaletteEditorToggle")==0)
    {
        if(cbData->set)
            Vrui::popupPrimaryWidget(transFuncEditor);
        else
            Vrui::popdownPrimaryWidget(transFuncEditor);
    }
    else if(strcmp(cbData->toggle->getName(),"ShowRenderSettingToggle")==0)
    {
        if(cbData->set)
            {
            /* Open the render dialog at the same position as the main menu: */
            Vrui::popupPrimaryWidget(RenderSettingDlg);
            }
        else
            {
            /* Close the render dialog: */
            Vrui::popdownPrimaryWidget(RenderSettingDlg);
            }
    }
}
void RayCastingVis::savePaletteCallback(Misc::CallbackData* cbData)
{
    if(Vrui::isMaster())
        {
        try
            {
            char numberedFileName[40];
            transFuncEditor->savePalette(Misc::createNumberedFileName("SavedPalette.pal",4,numberedFileName));
            }
        catch(std::runtime_error)
            {
            /* Ignore errors and carry on: */
            }
        }
}
void RayCastingVis::renderDialogCloseCallback(Misc::CallbackData* cbData)
{
//	showRenderDialogToggle->setToggle(false);
}
void RayCastingVis::renderDlgSlideChangeCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    if(strcmp(cbData->slider->getName(),"AmbientSlider")==0)
    {
//		surfaceTransparent=cbData->value<1.0;
        ambinetCoE = cbData->value;
        std::cout<<"set ambient coefficient"<<std::endl;
    }
    else if(strcmp(cbData->slider->getName(),"SpecularSlider")==0)
    {
        specularCoE = cbData->value;
        std::cout<<"set specular coefficient"<<std::endl;
    }
    else if(strcmp(cbData->slider->getName(),"DiffuseSlider")==0)
    {
        diffuseCoE = cbData->value;
        std::cout<<"set diffuse coefficient"<<std::endl;
    }
    else if(strcmp(cbData->slider->getName(),"StepSizeSlider")==0)
    {
        stepSize = cbData->value;
        std::cout<<"set step size"<<std::endl;
    }
}

void RayCastingVis::renderDlgToggleChangeCallback(GLMotif::ToggleButton::ValueChangedCallbackData *cbData)
{
    if(strcmp(cbData->toggle->getName(),"lightFlag")==0)
    {
        LightFlag=cbData->set;
        std::cout<<"Light Setting change"<<std::endl;
    }

}

void RayCastingVis::loadElementsCallback(Misc::CallbackData*)
    {

        /* Create a file selection dialog to select an element file: */
        GLMotif::FileSelectionDialog* fsDialog=new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),"Load Visualization Elements...",Vrui::openDirectory("./data"),".raw");
        fsDialog->getOKCallbacks().add(this,&RayCastingVis::loadElementsOKCallback);
        fsDialog->getCancelCallbacks().add(this,&RayCastingVis::loadElementsCancelCallback);
        Vrui::popupPrimaryWidget(fsDialog);
    }

void RayCastingVis::loadElementsOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
    {
    try
        {
        /* Determine the type of the element file: */
        if(Misc::hasCaseExtension(cbData->selectedFileName,".raw"))
            {
            /* Load the ASCII elements file: */
//            loadElements(cbData->selectedDirectory->getPath(cbData->selectedFileName).c_str(),true);
            }
        }
    catch(std::runtime_error err)
        {
        std::cerr<<"Caught exception "<<err.what()<<" while loading element file"<<std::endl;
        }

    /* Destroy the file selection dialog: */
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
    }

void RayCastingVis::loadElementsCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
    {
    /* Destroy the file selection dialog: */
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
    }

void RayCastingVis::bindPreIntShader(DataItem *dataItem) const
{
//    dataItem->initialPreIntBuffer(colorMap);
    /* Set the sampling step size: */
    glUniform1fARB(dataItem->deltastepLoc,stepSize*cellSize);
    glUniform1fARB(dataItem->OCSizeLoc,stepSize*cellSize);

    /* Bind the transfer function texture: */
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_2D,dataItem->preIntTextureID);
    glUniform1iARB(dataItem->transFuncTexLoc,1);

//    GLint index = dataItem->preIntShader.
//    glBindVertexArray(dataItem->vertexArrayObjectID);
//    glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectID);
//    glEnableVertexAttribArrayARB(index);
//    glVertexAttribPointerARB(index,2, GL_FLOAT,false,0,NULL);
//    glBindVertexArray(0);
//    glBindBufferARB(GL_ARRAY_BUFFER, 0);

}
void RayCastingVis::unbindPreIntShader(DataItem *dataItem) const
{
    /* Unbind the transfer function texture: */
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glBindTexture(GL_TEXTURE_2D,0);
}
void RayCastingVis::drawPreIntTexture(GLContextData& contextData) const
{
    DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
    const Vrui::DisplayState& vds=Vrui::getDisplayState(contextData);
    if(dataItem->vertexBufferObjectID>0)
    {
        std::vector<Point2D> preIntVertices;
        preIntVertices.push_back(Point2D(-1.0,-1.0));
        preIntVertices.push_back(Point2D(-1.0,1.0));
        preIntVertices.push_back(Point2D(1.0,1.0));
        preIntVertices.push_back(Point2D(1.0,-1.0));

        int width = vds.viewport[0];
        int height = vds.viewport[1];
//        std::cout<<"Screen viewport change: "<<width<<" "<<height<<std::endl;
        glViewport(0,0,256,256);
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,dataItem->preIntFramebufferID);
        glDisable(GL_BLEND);
        glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
        dataItem->preIntShader.useProgram();
        bindPreIntShader(dataItem);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectID);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB,preIntVertices.size()*sizeof(Point2D),0,GL_STATIC_DRAW_ARB);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);
        glBindBufferARB(GL_ARRAY_BUFFER, 0);
        unbindPreIntShader(dataItem);
        GLShader::disablePrograms();
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
        glEnable(GL_BLEND);
        glViewport(vds.viewport[0],vds.viewport[1],vds.viewport[2],vds.viewport[3]);
    }
}

VRUI_APPLICATION_RUN(RayCastingVis)
