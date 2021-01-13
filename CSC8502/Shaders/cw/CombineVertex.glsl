#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;
in vec2 texCoord;

out Vertex{
	vec2 texCoord;
} OUT;

void main(void)	{
	OUT.texCoord = texCoord;
	gl_Position = vec4(position, 1);
}