#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform sampler2D mergeTex;
uniform sampler2D mergeBump;
uniform sampler2D stencilTex;
uniform sampler2D shadowTex;

uniform vec4 lightColour;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform float lightRadius;

in Vertex{
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
} IN;

out vec4 fragColour[2];

void main(void){
	
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

	vec4 diffuse = texture(diffuseTex, IN.texCoord);
	vec3 bumpNormal = texture(bumpTex, IN.texCoord).xyz;
	bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));

	float lambert = max(dot(incident, bumpNormal), 0);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);

	float shadow = 1.0;
	vec3 shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
	if(abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f){
		vec3 biasCoord = shadowNDC * 0.5 + 0.5;
		float shadowZ = texture(shadowTex, biasCoord.xy).x;
		if(shadowZ < biasCoord.z){
			shadow = 0.75;
		}
	}

	float stencilAlpha = textureSize(stencilTex, 0).x > 1 ? texture(stencilTex, IN.texCoord).r : 1;
	vec4 merge = texture(mergeTex, IN.texCoord);
	vec4 surface = (merge * (1-stencilAlpha)) + (diffuse * stencilAlpha);
	fragColour[0] = (surface * lightColour);
	fragColour[0] *= lambert * attenuation;
	fragColour[0] += (lightColour * specFactor) * attenuation * 0.33;
	fragColour[0] += surface * 0.1;
	fragColour[0] *= shadow;
	fragColour[0].a = diffuse.a;

	vec3 normal = texture2D(bumpTex, IN.texCoord).rgb * 2.0 - 1.0;
	normal = normalize(TBN * normalize(normal));
	vec3 normalStencil = texture2D(mergeBump, IN.texCoord).rgb * 2.0 - 1.0;
	normalStencil = normalize(TBN * normalize(normalStencil));

	fragColour[1] = vec4(((normalStencil.xyz * (1 - stencilAlpha)) + (normal.xyz * stencilAlpha)) * 0.5 + 0.5, 1.0);
}