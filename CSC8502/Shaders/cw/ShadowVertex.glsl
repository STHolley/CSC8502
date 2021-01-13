#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform mat4 joints[128];
in vec4 jointWeights;
in ivec4 jointIndices;

in vec3 position;

void main(void)	{

	vec4 localPos = vec4(position, 1.0);
	vec4 skelPos = vec4(0,0,0,0);

	for(int i = 0; i < 4; i++){
		int jointIndex = jointIndices[i];
		float jointWeight = jointWeights[i];

		skelPos += joints[jointIndex] * localPos * jointWeight;
	}
	vec4 worldPos = modelMatrix * vec4(skelPos.xyz, 1.0);
	mat4 mvp = projMatrix * viewMatrix * modelMatrix;
	gl_Position = mvp * vec4(skelPos.xyz, 1);

	gl_Position	  = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}