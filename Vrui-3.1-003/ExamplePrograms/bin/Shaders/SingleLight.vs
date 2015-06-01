/***********************************************************************
Vertex shader for multi-pass per-fragment lighting using the same
formulae as fixed-functionality OpenGL.
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

varying vec4 shadow;
varying vec4 ambient,diffuse,specular;
varying vec3 normal,lightDir,halfVector;
varying float lightAttenuation;

void main()
	{
	vec4 vertexEc;
	float lightDist;
	
	/* Calculate per-source ambient light term: */
	ambient=gl_LightSource[0].ambient*gl_FrontMaterial.ambient;
	
	/* Compute the normal vector: */
	normal=normalize(gl_NormalMatrix*gl_Normal);
	
	/* Compute the light direction (works both for directional and point lights): */
	vertexEc=gl_ModelViewMatrix*gl_Vertex;
	lightDir=gl_LightSource[0].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[0].position.w;
	
	/* Calculate the light attenuation: */
	lightDist=length(lightDir);
	lightAttenuation=(gl_LightSource[0].quadraticAttenuation*lightDist+gl_LightSource[0].linearAttenuation)*lightDist+gl_LightSource[0].constantAttenuation;
	
	/* Normalize the light direction: */
	lightDir=normalize(lightDir);
	
	/* Compute the per-source diffuse light term: */
	diffuse=gl_LightSource[0].diffuse*gl_FrontMaterial.diffuse;
	
	/* Compute the half-vector: */
	halfVector=normalize(normalize(-vertexEc.xyz)+lightDir);
	
	/* Compute the per-source specular light term: */
	specular=gl_LightSource[0].specular*gl_FrontMaterial.specular;
	
	/* Calculate the shadow texture position: */
	shadow=gl_TextureMatrix[0]*vertexEc;
	
	/* Use standard vertex position: */
	gl_Position=ftransform();
	}
