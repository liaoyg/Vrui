#version 330

in vec2 Vertex;
layout (location = 1) in vec4 texCoord;
out vec3 CameraDir;
out vec3 CameraPos;
out vec3 Normalize;
out vec3 Vertex3;

uniform float aspect;
uniform float cotFOV;
uniform mat4 invModelView;
uniform mat4 transform;

void main(void)
{
	vec3 dir;
	dir.x = (Vertex.x * 2.0 - 1.0) * aspect;
	dir.y = Vertex.y * 2.0 - 1.0;
	dir.z = -cotFOV;
	CameraDir = mat3(invModelView) * normalize(dir);
	CameraPos = (invModelView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;


	gl_Position = transform * vec4(Vertex, 0.0, 1.0);
}
