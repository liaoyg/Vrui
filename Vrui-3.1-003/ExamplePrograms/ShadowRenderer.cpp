/***********************************************************************
ShadowRenderer - Test program for rendering shadows using shadow maps.
Copyright (c) 2008-2015 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <vector>
#include <iostream>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLLightTemplates.h>
#include <GL/GLColor.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLARBDepthTexture.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Application.h>

class ShadowRenderer:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	typedef float Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Vector<Scalar,3> Vector;
	typedef Geometry::Box<Scalar,3> Box;
	
	struct ShadowLight // Structure for shadow-casting light sources
		{
		/* Embedded classes: */
		public:
		typedef Geometry::OrthonormalTransformation<Scalar,3> Transform;
		
		/* Elements: */
		public:
		Transform transform; // Light position and orientation
		GLColor<GLfloat,4> lightColor; // Overall light color
		Scalar pyramidSize[3]; // Size of light's coverage pyramid
		
		/* Methods: */
		void setLight(int lightSourceIndex) const; // Installs the light as the given OpenGL light source
		void multMatrix(const Box& boundingBox) const; // Loads the light's shadow projection matrix
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint displayListIds[3]; // Display lists containing the scene components
		bool haveShaders; // Flag if GLSL shaders are supported
		GLhandleARB globalAmbientShader; // Shader program for the first rendering pass: global ambient illumination
		GLhandleARB shadowPassShader; // Shader program for shadow rendering pass
		GLhandleARB singleLightShader; // Shader program for each enabled light source
		GLint singleLightShaderShadowTextureLocation; // Location of shadow texture uniform variable in single-light shader
		bool haveFramebufferObjects; // Flag if frame buffer objects are supported
		GLuint shadowDepthTextureObject; // Depth texture for the shadow rendering frame buffer
		GLuint shadowFramebufferObject; // Frame buffer object to render shadow maps
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Box boundingBox; // Bounding box of the scene
	std::vector<ShadowLight> shadowLights; // List of shadow light sources
	Vrui::InputDevice* lightDevice; // A virtual input device to move a light source
	ShadowLight* trackedLightSource; // Pointer to light source tracking the virtual input device
	GLsizei shadowBufferSize[2]; // Size of the shadow rendering frame buffer
	
	/* Constructors and destructors: */
	public:
	ShadowRenderer(int& argc,char**& argv);
	virtual ~ShadowRenderer(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/********************************************
Methods of class ShadowRenderer::ShadowLight:
********************************************/

void ShadowRenderer::ShadowLight::setLight(int lightSourceIndex) const
	{
	/* Enable the light source: */
	glEnableLight(lightSourceIndex);
	
	/* Set the light source's position: */
	glLightPosition(lightSourceIndex,transform.getOrigin());
	
	/* Set the light source's spot direction: */
	glLightSpotDirection(lightSourceIndex,-transform.getDirection(2));
	
	/* Set the light source's color: */
	glLightAmbient(lightSourceIndex,GLColor<GLfloat,4>(0.0f,0.0f,0.0f));
	glLightDiffuse(lightSourceIndex,lightColor);
	glLightSpecular(lightSourceIndex,lightColor);
	}

void ShadowRenderer::ShadowLight::multMatrix(const ShadowRenderer::Box& boundingBox) const
	{
	/* Find the extents of the domain bounding box along the light source's light direction: */
	Scalar min,max;
	min=max=transform.inverseTransform(boundingBox.getVertex(0))[2];
	for(int i=1;i<8;++i)
		{
		Scalar d=transform.inverseTransform(boundingBox.getVertex(i))[2];
		if(min>d)
			min=d;
		if(max<d)
			max=d;
		}
	
	/* Load the light source's coverage pyramid as a frustum: */
	Scalar far=-min;
	Scalar near=-max;
	if(near<pyramidSize[2])
		near=pyramidSize[2];
	if(far<near*Scalar(2))
		far=near*Scalar(2);
	Scalar s=near/pyramidSize[2];
	glFrustum(-pyramidSize[0]*s,pyramidSize[0]*s,-pyramidSize[1]*s,pyramidSize[1]*s,near,far);
	
	/* Go to the light source's local coordinate system: */
	glMultMatrix(Geometry::invert(transform));
	}

/*****************************************
Methods of class ShadowRenderer::DataItem:
*****************************************/

ShadowRenderer::DataItem::DataItem(void)
	:haveShaders(false),globalAmbientShader(0),shadowPassShader(0),singleLightShader(0),
	 haveFramebufferObjects(false),shadowDepthTextureObject(0),shadowFramebufferObject(0)
	{
	/* Initialize the display lists: */
	GLuint listBase=glGenLists(3);
	for(int i=0;i<3;++i)
		displayListIds[i]=listBase+GLuint(i);
	
	/* Check for shader support: */
	haveShaders=GLARBShaderObjects::isSupported()&&GLARBVertexShader::isSupported()&&GLARBFragmentShader::isSupported();
	if(haveShaders)
		{
		/* Initialize the extensions: */
		GLARBShaderObjects::initExtension();
		GLARBVertexShader::initExtension();
		GLARBFragmentShader::initExtension();
		}
	
	/* Check for frame buffer object support: */
	haveFramebufferObjects=GLARBDepthTexture::isSupported()&&GLEXTFramebufferObject::isSupported();
	if(haveFramebufferObjects)
		{
		/* Initialize the extension: */
		GLEXTFramebufferObject::initExtension();
		GLARBDepthTexture::initExtension();
		}
	}

ShadowRenderer::DataItem::~DataItem(void)
	{
	/* Delete the display lists: */
	glDeleteLists(displayListIds[0],3);
	
	if(haveShaders)
		{
		/* Delete the shaders: */
		glDeleteObjectARB(globalAmbientShader);
		glDeleteObjectARB(shadowPassShader);
		glDeleteObjectARB(singleLightShader);
		}
	
	if(haveFramebufferObjects)
		{
		/* Delete the frame buffer objects: */
		glDeleteFramebuffersEXT(1,&shadowFramebufferObject);
		glDeleteTextures(1,&shadowDepthTextureObject);
		}
	}

/*******************************
Methods of class ShadowRenderer:
*******************************/

ShadowRenderer::ShadowRenderer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 lightDevice(0),trackedLightSource(0)
	{
	/* Initialize the bounding box: */
	boundingBox=Box(Box::Point(-100.0f,-100.0f,0.0f),Box::Point(100.0f,100.0f,55.0f));
	
	/* Create some shadow light sources: */
	ShadowLight sl1;
	sl1.transform=ShadowLight::Transform::identity;
	sl1.transform*=ShadowLight::Transform::rotate(ShadowLight::Transform::Rotation::rotateAxis(Vector(1,-1,0),Math::rad(Scalar(30))));
	sl1.transform*=ShadowLight::Transform::translate(Vector(0,0,100));
	sl1.lightColor=GLColor<GLfloat,4>(0.7f,0.7f,0.7f);
	sl1.pyramidSize[0]=sl1.pyramidSize[1]=Scalar(1);
	sl1.pyramidSize[2]=Scalar(0.5);
	shadowLights.push_back(sl1);
	
	ShadowLight sl2;
	sl2.transform=ShadowLight::Transform::identity;
	sl2.transform*=ShadowLight::Transform::rotate(ShadowLight::Transform::Rotation::rotateAxis(Vector(1,0.5,0),Math::rad(Scalar(-45))));
	sl2.transform*=ShadowLight::Transform::translate(Vector(0,0,120));
	sl2.lightColor=GLColor<GLfloat,4>(0.7f,0.7f,0.7f);
	sl2.pyramidSize[0]=sl2.pyramidSize[1]=Scalar(1);
	sl2.pyramidSize[2]=Scalar(0.5);
	shadowLights.push_back(sl2);
	
	/* Create a virtual input device to move a light source: */
	trackedLightSource=&shadowLights.back();
	lightDevice=Vrui::addVirtualInputDevice("LightDevice",0,0);
	lightDevice->setTransformation(trackedLightSource->transform);
	Vrui::getInputGraphManager()->setNavigational(lightDevice,true);
	
	/* Set the shadow buffer size: */
	shadowBufferSize[1]=shadowBufferSize[0]=512;
	
	/* Initialize the navigation transformation: */
	Point center=Geometry::mid(boundingBox.min,boundingBox.max);
	center[2]=(boundingBox.min[2]*Scalar(3)+boundingBox.max[2]*Scalar(1))/Scalar(4);
	Scalar size=Math::div2(Geometry::dist(boundingBox.min,boundingBox.max));
	Vrui::setNavigationTransformation(Vrui::Point(center),Vrui::Scalar(size),Vrui::Vector(0,0.5,1.0));
	}

ShadowRenderer::~ShadowRenderer(void)
	{
	}

void ShadowRenderer::frame(void)
	{
	/* Update the tracked light source from the virtual input device: */
	Vrui::NavTransform lightT=lightDevice->getTransformation();
	lightT.leftMultiply(Vrui::getInverseNavigationTransformation());
	trackedLightSource->transform=ShadowLight::Transform(lightT.getTranslation(),lightT.getRotation());
	}

void ShadowRenderer::display(GLContextData& contextData) const
	{
	/* Get the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Access this window's display state: */
	const Vrui::DisplayState& displayState=Vrui::getDisplayState(contextData);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_POLYGON_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT);
	GLint currentFramebufferId;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&currentFramebufferId);
	
	/*********************************************************************
	First rendering pass: Global ambient illumination only
	*********************************************************************/
	
	/* Render the scene using the global ambient shader: */
	glUseProgramObjectARB(dataItem->globalAmbientShader);
	for(int i=0;i<3;++i)
		glCallList(dataItem->displayListIds[i]);
	
	/*********************************************************************
	Second rendering pass: Add local illumination for every light source
	*********************************************************************/
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	
	for(std::vector<ShadowLight>::const_iterator slIt=shadowLights.begin();slIt!=shadowLights.end();++slIt)
		{
		if(dataItem->haveFramebufferObjects)
			{
			/* Render the scene from the light source's point of view: */
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,dataItem->shadowFramebufferObject);
			glViewport(0,0,shadowBufferSize[0],shadowBufferSize[1]);
			glDepthMask(GL_TRUE);
			glCullFace(GL_FRONT);
			glClear(GL_DEPTH_BUFFER_BIT);
			
			/* Set up the projection matrix for the shadow pass shader: */
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glLoadIdentity();
			slIt->multMatrix(boundingBox);
			glMultMatrix(Geometry::invert(displayState.modelviewNavigational));
			glMatrixMode(GL_MODELVIEW);
			
			/* Render the scene using the shadow pass shader: */
			glUseProgramObjectARB(dataItem->shadowPassShader);
			for(int i=0;i<3;++i)
				glCallList(dataItem->displayListIds[i]);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,currentFramebufferId);
			glViewport(displayState.viewport[0],displayState.viewport[1],displayState.viewport[2],displayState.viewport[3]);
			glDepthMask(GL_FALSE);
			glCullFace(GL_BACK);
			
			/* Set up the light source: */
			slIt->setLight(0);
			
			/* Bind the shadow texture: */
			glBindTexture(GL_TEXTURE_2D,dataItem->shadowDepthTextureObject);
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glTranslated(0.5,0.5,0.5);
			glScaled(0.5,0.5,0.5);
			slIt->multMatrix(boundingBox);
			glMultMatrix(Geometry::invert(displayState.modelviewNavigational));
			glMatrixMode(GL_MODELVIEW);
			
			/* Render the scene using the single light source shader: */
			glUseProgramObjectARB(dataItem->singleLightShader);
			glUniform1iARB(dataItem->singleLightShaderShadowTextureLocation,0);
			for(int i=0;i<3;++i)
				glCallList(dataItem->displayListIds[i]);
			
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glBindTexture(GL_TEXTURE_2D,0);
			}
		else
			{
			/* Set up the light source: */
			slIt->setLight(0);
			glUseProgramObjectARB(dataItem->singleLightShader);
			
			/* Render the scene: */
			for(int i=0;i<3;++i)
				glCallList(dataItem->displayListIds[i]);
			}
		}
	
	/* Disable shader: */
	glUseProgramObjectARB(0);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void ShadowRenderer::initContext(GLContextData& contextData) const
	{
	/* Create a data item and add it to the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the floor model: */
	glNewList(dataItem->displayListIds[0],GL_COMPILE);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.5f,0.5f,0.5f)));
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex3f(-100.0f,-100.0f,0.0f);
	glVertex3f( 100.0f,-100.0f,0.0f);
	glVertex3f( 100.0f, 100.0f,0.0f);
	glVertex3f(-100.0f, 100.0f,0.0f);
	glEnd();
	glEndList();
	
	/* Create the sphere model: */
	glNewList(dataItem->displayListIds[1],GL_COMPILE);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(1.0f,0.0f,0.0f),GLMaterial::Color(1.0f,1.0f,1.0f),25.0f));
	glPushMatrix();
	glTranslatef(0.0f,0.0f,30.0f);
	glDrawSphereIcosahedron(25.0f,16);
	glPopMatrix();
	glEndList();
	
	/* Create the cylinder model: */
	glNewList(dataItem->displayListIds[2],GL_COMPILE);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.5f,0.5f,1.0f),GLMaterial::Color(0.5f,0.5f,0.5f),5.0f));
	glPushMatrix();
	glTranslatef(60.0f,-30.0f,25.0f);
	glDrawCylinder(15.0f,50.0f,64);
	glPopMatrix();
	glEndList();
	
	if(dataItem->haveShaders)
		{
		/* Create the global ambient shader: */
		{
		GLhandleARB vertexShader=glCompileVertexShaderFromFile("Shaders/GlobalAmbient.vs");
		GLhandleARB fragmentShader=glCompileFragmentShaderFromFile("Shaders/GlobalAmbient.fs");
		dataItem->globalAmbientShader=glLinkShader(vertexShader,fragmentShader);
		glDeleteObjectARB(vertexShader);
		glDeleteObjectARB(fragmentShader);
		}
		
		/* Create the shadow pass shader: */
		{
		GLhandleARB vertexShader=glCompileVertexShaderFromFile("Shaders/ShadowPass.vs");
		GLhandleARB fragmentShader=glCompileFragmentShaderFromFile("Shaders/ShadowPass.fs");
		dataItem->shadowPassShader=glLinkShader(vertexShader,fragmentShader);
		glDeleteObjectARB(vertexShader);
		glDeleteObjectARB(fragmentShader);
		}
		
		/* Create the single-light source shader: */
		{
		GLhandleARB vertexShader=glCompileVertexShaderFromFile("Shaders/SingleLight.vs");
		GLhandleARB fragmentShader=glCompileFragmentShaderFromFile("Shaders/SingleLight.fs");
		dataItem->singleLightShader=glLinkShader(vertexShader,fragmentShader);
		dataItem->singleLightShaderShadowTextureLocation=glGetUniformLocationARB(dataItem->singleLightShader,"shadowTexture");
		glDeleteObjectARB(vertexShader);
		glDeleteObjectARB(fragmentShader);
		}
		}
	
	if(dataItem->haveFramebufferObjects)
		{
		/* Generate a depth texture for shadow rendering: */
		glGenTextures(1,&dataItem->shadowDepthTextureObject);
		glBindTexture(GL_TEXTURE_2D,dataItem->shadowDepthTextureObject);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC_ARB,GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE_ARB,GL_INTENSITY);
		glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,shadowBufferSize[0],shadowBufferSize[1],0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
		glBindTexture(GL_TEXTURE_2D,0);
		
		/* Generate the shadow rendering frame buffer: */
		glGenFramebuffersEXT(1,&dataItem->shadowFramebufferObject);
		
		/* Attach the depth texture to the frame buffer object: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,dataItem->shadowFramebufferObject);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,dataItem->shadowDepthTextureObject,0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		}
	}

VRUI_APPLICATION_RUN(ShadowRenderer)
