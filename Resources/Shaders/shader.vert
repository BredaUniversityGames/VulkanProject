#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform ModelMatrix 
{
    mat4 model;
  
} matrix;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBN;

void main()
{
    gl_Position = ubo.proj * ubo.view * matrix.model * vec4(inPosition, 1.0);
    fragColor = inColor;
   
   	vec3 normal = normalize(inNormal * inverse(mat3(matrix.model)));
	vec3 tangent = normalize(inTangent.xyz * inverse(mat3(matrix.model)));
	vec3 biTangent = normalize(cross(inNormal, inTangent.xyz) * inverse(mat3(matrix.model)));
	TBN = mat3(tangent, biTangent, normal);

    fragTexCoord = inTexCoord;
}