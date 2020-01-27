#version 150

in float alphaVal;
out vec4 fColor;

void main() {
    fColor = vec4(0.5,0.0,1.0,alphaVal); 
}