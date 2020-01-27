#version 150

in vec4 vPosition;

out float alphaVal;

void main()
{
  alphaVal = vPosition.z;
  gl_Position = vPosition;
}