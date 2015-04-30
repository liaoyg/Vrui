#include <vector>
#include <string>
#include <iostream>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>	
#include <Vrui/DisplayState.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Application.h>

class VolumeRayCasting:public Vrui::Application,public GLObject
{
 private:
  typedef float Unit;
  typedef Geometry::Vector<Unit,2> Vector2f;
  typedef Geometry::Vector<int,3> Vector3i;
  typedef Geometry::Vector<Unit,3> Vector3f;
  typedef Geometry::Vector<Unit,4> Vector4f;

  //Vrui::Vector VolumeDim = Vrui::Vector(32,32,32);
  Vector3i VolumeDim;
  std::vector<Vector4f> transFuncData;

  struct DataItem: public GLObject::DataItem
  {
    /* Elements: */
  public:
    GLuint displayListIds[1]; // Display lists containing the scene components
    bool haveShaders; // Flag if GLSL shaders are supported
    GLhandleARB rayCastingShader; // Shader program for volume rendering ray casting
    GLuint rectVArrayBufferId;
    GLuint rectVerticesArrayId;
    
    GLuint volumeTex;
    GLuint transferFuncTex;
		
    /* Constructors and destructors: */
    DataItem(void);
    virtual ~DataItem(void);		
  };
 public:
  VolumeRayCasting(int& argc, char**& argv);
  virtual ~VolumeRayCasting(void);

 private:
  void SetTexture(GLuint shaderId, const GLchar *name, GLuint texture, GLenum type) const;
  /* Methods from Vrui::Application: */
  virtual void frame(void);
  virtual void display(GLContextData& contextData) const;
	
  /* Methods from GLObject: */
  virtual void initContext(GLContextData& contextData) const;
};


