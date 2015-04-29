#version 330

in vec3 Vertex;
//out vec3 CameraDir;
//out vec3 CameraPos;

//uniform float aspect;
//uniform float cotFOV;
//uniform mat4 invModelView;
uniform mat4 modelview;
uniform mat4 projectM;

void main(void)
{
//        vec3 dir;
//        dir.x = (Vertex.x * 2.0 - 1.0) * aspect;
//        dir.y = Vertex.y * 2.0 - 1.0;
//        dir.z = -cotFOV;
//        CameraDir = mat3(invModelView) * normalize(dir);
//        CameraPos = (invModelView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
        gl_Position = projectM * modelview * vec4(Vertex, 1.0);
}
