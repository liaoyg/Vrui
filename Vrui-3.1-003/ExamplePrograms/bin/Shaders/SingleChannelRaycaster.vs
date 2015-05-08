
uniform vec3 mcScale;
uniform vec3 mcOffset;

varying vec3 mcPosition;
varying vec3 dcPosition;

void main()
	{
	/* Store the model-coordinate vertex position: */
	mcPosition=gl_Vertex.xyz/gl_Vertex.w;
	
	/* Store the data-coordinate vertex position: */
	dcPosition=mcPosition*mcScale+mcOffset;
	
	/* Calculate the clip-coordinate vertex position: */
	gl_Position=ftransform();
	
	/* Ensure that no vertices are in front of the front plane, which can happen due to rounding: */
	if(gl_Position[2]<-gl_Position[3])
		gl_Position[2]=-gl_Position[3];
	}
