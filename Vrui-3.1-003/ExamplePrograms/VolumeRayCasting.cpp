
#include <fstream>
#include <Math/Math.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLExtension.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBVertexArrayObject.h>
#include <GL/Extensions/GLARBVertexProgram.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLEXTGpuShader4.h>
#include "VolumeRayCasting.h"
#include <errno.h>


VolumeRayCasting::DataItem::DataItem(void)
  :haveShaders(false),rayCastingShader(0),volumeTex(0),transferFuncTex(0),
   rectVArrayBufferId(0),rectVerticesArrayId(0)
{
  GLuint listBase=glGenLists(1);
  for(int i=0;i<1;++i)
    displayListIds[i]=listBase+GLuint(i);

  //initialize extension
  GLARBVertexBufferObject::initExtension();
  GLARBVertexArrayObject::initExtension();
  GLARBVertexProgram::initExtension();
  GLEXTGpuShader4::initExtension();
  	
  /* Check for shader support: */
  haveShaders=GLARBShaderObjects::isSupported()&&GLARBVertexShader::isSupported()&&GLARBFragmentShader::isSupported();
  if(haveShaders)
    {
      /* Initialize the extensions: */
      GLARBShaderObjects::initExtension();
      GLARBVertexShader::initExtension();
      GLARBFragmentShader::initExtension();
    }
  
  glGenTextures(1,&volumeTex);
  glGenTextures(1,&transferFuncTex);

}


VolumeRayCasting::DataItem::~DataItem(void)
{
  /* Delete the display lists: */
  glDeleteLists(displayListIds[0],3);
	
  if(haveShaders)
    {
      /* Delete the shaders: */
      glDeleteObjectARB(rayCastingShader);
    }
}

VolumeRayCasting::VolumeRayCasting(int& argc, char**& argv)
  :Vrui::Application(argc,argv)
{
  VolumeDim = Vector3i(32, 32, 32);
  // prepare transfer function data
  transFuncData.clear();
  for(int i = 0; i< 512; i++)
    {
      if(i < 64)
	transFuncData.push_back(Vector4f(1.0f, 0.5f, 0.5f, 0.0f));
      else if(i<128)
	transFuncData.push_back(Vector4f(1.0f, 1.0f, 0.0f, 0.001f*i));
      else if(i<196)
	transFuncData.push_back(Vector4f(0.0f, 1.0f, 0.0f, 0.001f*i));
      else if(i<312)
	transFuncData.push_back(Vector4f(0.2f, 1.0f, 1.0f, 0.0001f*i));
      else if(i<400)
	transFuncData.push_back(Vector4f(0.1f, 0.3f, 1.0f, 0.0001f*i));
      else
	transFuncData.push_back(Vector4f(1.0f, 0.0f, 1.0f, 0.0002f*i));
    }
}

VolumeRayCasting::~VolumeRayCasting(void)
{
  transFuncData.clear();
}

void VolumeRayCasting::initContext(GLContextData& contextData) const
{
  DataItem* dataItem=new DataItem();
  contextData.addDataItem(this,dataItem);

  const char* datasetName = "bin/data/BostonTeapot.raw";
  int volumesize = 256*256*256;
  /*load sample data*/
  std::vector<unsigned char> volumeData;
  volumeData.resize(volumesize);
  std::ifstream ifs(datasetName, std::ios::binary);

  if(!ifs.is_open())
    {
      /* fail to open dataset file */
      std::cout<<"fail to open dataset file: "<<strerror(errno)<<std::endl;
      return;
    }
  ifs.read(reinterpret_cast<char *>(&volumeData.front()), volumesize * sizeof(float));
  ifs.close();

  /* Select the Volume Rendering texture object: */
  glBindTexture(GL_TEXTURE_3D,dataItem->volumeTex);
	
  /* Upload the Volume Rendering texture image: */
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, 32, 32, 32, 0,  GL_RED, GL_UNSIGNED_BYTE, &volumeData.front());
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	
  /* Protect the Volume Rendering texture object: */
  glBindTexture(GL_TEXTURE_3D,0);

  /* Select the Tansfer Function texture object: */
  glBindTexture(GL_TEXTURE_2D,dataItem->transferFuncTex);
	
  /* Upload the Tansfer Function texture image: */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, transFuncData.size(), 1, 0, GL_RGBA, GL_FLOAT, &transFuncData.front());
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	
  /* Protect the Tansfer Function texture object: */
  glBindTexture(GL_TEXTURE_2D,0);

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glDisable(GL_DEPTH_TEST);
  glColorMask(true, true, true, true);
  glDepthMask(true);
  glEnable(GL_MULTISAMPLE);

  if(dataItem->haveShaders)
    {
      {
    GLhandleARB vertexShader=glCompileVertexShaderFromFile("bin/Shaders/VolumeRayCasting.vert");
    GLhandleARB fragmentShader=glCompileFragmentShaderFromFile("bin/Shaders/VolumeRayCasting.frag");
	dataItem->rayCastingShader=glLinkShader(vertexShader,fragmentShader);
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(fragmentShader);
      }	
    }

  std::vector<Vector2f> rectVertices;
  rectVertices.push_back(Vector2f(0.0f,0.0f));
  rectVertices.push_back(Vector2f(0.0f,1.0f));
  rectVertices.push_back(Vector2f(1.0f,1.0f));
  rectVertices.push_back(Vector2f(1.0f,0.0f));
  glGenBuffersARB(1,&(dataItem->rectVArrayBufferId));
  glBindBufferARB(GL_ARRAY_BUFFER, dataItem->rectVArrayBufferId);
  glBufferDataARB(GL_ARRAY_BUFFER, rectVertices.size()*sizeof(Vector2f), &rectVertices.front(),GL_STATIC_DRAW);
  glBindBufferARB(GL_ARRAY_BUFFER, 0);
  glGenVertexArrays(1,&(dataItem->rectVerticesArrayId));
  GLuint index = glGetAttribLocationARB(dataItem->rayCastingShader, "Vertex");
  glBindVertexArray(dataItem->rectVerticesArrayId);
  glBindBufferARB(GL_ARRAY_BUFFER, dataItem->rectVArrayBufferId);
  glEnableVertexAttribArrayARB(index);
  glVertexAttribPointerARB(index,2, GL_FLOAT,false,0,NULL);
  glBindVertexArray(0);
  glBindBufferARB(GL_ARRAY_BUFFER, 0);
  
  glNewList(dataItem->displayListIds[0],GL_COMPILE);
  glBindVertexArray(dataItem->rectVerticesArrayId);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);
  glEndList();

}

void VolumeRayCasting::SetTexture(GLuint shaderId, const GLchar *name, GLuint texture, GLenum type)
{
  GLint location = glGetUniformLocationARB(shaderId, name);
  if(location < 0)
    return;
  
  GLuint index = -1;
  //glGetUniformuiv(shaderId, location, &index);
  glGetUniformuivEXT(shaderId, location, &index);
  if(index == -1)
    {
      index = 0;
      glUseProgramObjectARB(shaderId);
      glUniform1iARB(location, index);
    }

  glActiveTexture(GL_TEXTURE0 + index);
  glBindTexture(type, texture);
}

void VolumeRayCasting::display(GLContextData& contextData) const
{
  /* Get the data item: */
  DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);

  /* Access this window's display state: */
  const Vrui::DisplayState& displayState=Vrui::getDisplayState(contextData);

  glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_POLYGON_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // SetTexture(dataItem->rayCastingShader, "volumeTex", dataItem->volumeTex, GL_TEXTURE_3D);
  // SetTexture(dataItem->rayCastingShader, "transferFuncTex", dataItem->transferFuncTex, GL_TEXTURE_2D);

  glUseProgramObjectARB(dataItem->rayCastingShader);
  for(int i=0;i<1;++i)
    glCallList(dataItem->displayListIds[i]);
  glUseProgramObjectARB(0);

  glPopAttrib();
}

VRUI_APPLICATION_RUN(VolumeRayCasting)
