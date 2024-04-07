#version 450

layout (binding = 2) uniform sampler2D diffuse;
layout (binding = 3) uniform sampler2D normal;
layout (binding = 4) uniform sampler2D metallic;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 outColor;



void main() 
{
    float LightIntensity = 0.5;
    vec3 lightDirection = normalize(vec3(0.,0.,-1.));

    vec4 diffuseColor = texture(diffuse, fragTexCoord);
    vec4 normalColor = texture(normal, fragTexCoord);
    vec4 metallicColor = texture(metallic, fragTexCoord);

    float ambientintensity = 0.2;
    vec3 normal;
    // Not correct
    normal = normalColor.xyz * 2.0 - 1.0;
    normal = normalize(normal * TBN);

    vec3 lightcolor = vec3(1.) * max(0.,dot(-lightDirection, normal));
    vec3 ambientcolor = vec3(1.) * ambientintensity;
    vec3  color1 = diffuseColor.xyz * (lightcolor + ambientcolor);
    outColor = vec4(color1, 1.);
}
