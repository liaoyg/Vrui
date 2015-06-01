/***********************************************************************
Fragment shader for multi-pass per-fragment lighting using the same
formulae as fixed-functionality OpenGL
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

uniform sampler2DShadow shadowTexture;

varying vec4 shadow;
varying vec4 ambient,diffuse,specular;
varying vec3 normal,lightDir,halfVector;
varying float lightAttenuation;

vec4 offsetLookup(sampler2DShadow shadowMap,vec4 loc,vec2 offset)
	{
	vec2 shadowMapScale=vec2(1.0/512.0,1.0/512.0);
	return shadow2DProj(shadowMap,vec4(loc.xy+offset*shadowMapScale*loc.w,loc.z,loc.w));
	}

void main()
	{
	vec4 color;
	vec3 nNormal;
	float nl;
	
	/* Calculate the per-source ambient light term: */
	color=ambient;
	
	/* Compute the diffuse lighting angle: */
	nNormal=normalize(normal);
	nl=dot(nNormal,normalize(lightDir));
	if(nl>0.0)
		{
		float nhv;
		
		/* Calculate the per-source diffuse light term: */
		color+=diffuse*nl;
		
		/* Calculate the specular lighting angle: */
		nhv=max(dot(nNormal,normalize(halfVector)),0.0);
		color+=specular*pow(nhv,gl_FrontMaterial.shininess);
		}
	
	/* Attenuate the per-source light terms: */
	color=color*(1.0/lightAttenuation);
	
	/* Attenuate the color by the shadow texture: */
	vec4 shadowSum=vec4(0.0,0.0,0.0,0.0);
	float x,y;
	for(y=-1.5;y<2.0;y+=1.0)
		for(x=-1.5;x<2.0;x+=1.0)
			shadowSum+=offsetLookup(shadowTexture,shadow,vec2(x,y));
	shadowSum/=16.0;
	color*=shadowSum;
	//color*=shadow2DProj(shadowTexture,shadow).r;
	
	/* Set the fragment color: */
	gl_FragColor=color;
	}
