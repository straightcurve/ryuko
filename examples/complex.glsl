#version 450

varying vec3 vPosition;
varying float vUVx;
varying vec3 vNormal;
varying float vUVy;
varying vec4 vColor;

varying mediump vec2 UV;
varying mediump vec4 Color;
varying mediump vec4 Position;
varying mediump vec3 Normal;
varying flat uint InstanceIndex;
varying mediump vec4 WorldPosition;

vec4 vert()
{
    uint transformIndex = Constants.tbb.batch[Constants.transformBatchStartIndex + gl_InstanceIndex];
    mat4 modelMatrix = Constants.tb.transforms[transformIndex];
    Material m = Constants.mb.materials[Constants.materialIndex];

    UV = vec2(vUVx, vUVy);

    vec3 displacedPosition = vPosition;
    if (true) {
        float displacement = texture(textures[m.displacement], UV).r;
        displacement = displacement * m.displacementScale + m.displacementBias;
        displacedPosition += vNormal * displacement;
    }

    // Transform the vertex from model space to world space
    vec4 worldPosition = modelMatrix * vec4(displacedPosition, 1.0f);
    WorldPosition = worldPosition;

    // Transform the world position to camera space (view space)
    vec4 clipSpacePosition = camera.viewproj * worldPosition;
    Position = clipSpacePosition;

    // If the model is scaled non-uniformly, the normals should be transformed by the normal matrix, which is the inverse transpose of the model matrix. You can calculate it in the vertex shader:
    // This ensures correct lighting and shading when the model matrix contains non-uniform scaling.
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 transformedNormal = normalize(normalMatrix * vNormal);
    Normal = transformedNormal;

    Color = vColor;
    InstanceIndex = gl_InstanceIndex;

    return Position;
}

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 getNormalFromMap(uint normal);

const float PI = 3.14159265359;

vec4 frag()
{
    uint lightCount = Constants.lightCount;
    Material m = Constants.mb.materials[Constants.materialIndex];

    vec3 cameraPosition = camera.position.xyz;
    vec3 worldPosition = WorldPosition.xyz; // Position in world space

    // Sample textures and apply gamma correction for albedo
    vec3 albedo = pow(texture(textures[m.albedo], UV).rgb * m.albedoFactor.rgb, vec3(2.2));
    vec3 normal = getNormalFromMap(m.normal);
    //    float metallic = texture(textures[m.metallic], UV).r;
    float metallic = 0.f;
    //    float roughness = texture(textures[m.roughness], UV).r;
    float roughness = 0.4f;
    float ao = texture(textures[m.ambientOcclusion], UV).r;

    // Normal and view vector
    vec3 normalMapDirection = normalize(normal);
    vec3 normalDirection = normalize(Normal);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);

    // F0 is the base reflectivity at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Initialize outgoing radiance
    vec3 Lo = vec3(0.0);

    vec3 _ambient = vec3(0.03) * albedo * ao;

    if (m.emissive.strength > 0) {
        vec3 emission = m.emissive.color * m.emissive.strength;
        Lo += emission;
    }

    //  Iterate over each light source
    for (int i = 0; i < lightCount; ++i)
    {
        Light light = Constants.lb.lights[i];
        if (light.kind != 1) {
            continue;
        }

        vec3 lightDirection = normalize(light.position - worldPosition);
        vec3 H = normalize(viewDirection + lightDirection);

        //  new attenuation
        float intensity0 = light.intensity;
        //        float distance = length(light.position - worldPosition);
        float k = 0.1;
        //        float attenuation = intensity0 / (distance * distance + k);
        //        float attenuation = 1 / (distance * distance);

        // Example: Light attenuation based on watts
        float watts = light.intensity; // Light power consumption in watts
        float lumens = watts * 100.0; // Calculate lumens based on watts and efficiency

        // Attenuation based on distance (inverse square law)
        float distance = length(light.position - worldPosition);

        // Use a falloff factor to control the speed of attenuation
        float falloffFactor = 0.1; // This is an adjustable parameter to tweak attenuation
        float attenuation = lumens / (distance * distance + falloffFactor);

        // Optional: Add a maximum attenuation limit for visual control
        float maxAttenuation = 5.0; // Cap the maximum brightness at a reasonable level
        attenuation = min(attenuation, maxAttenuation); // This prevents the light from becoming too intense at close distances

        vec3 radiance = light.color * attenuation;

        //  cos theta
        float NdotL = max(dot(normalDirection, lightDirection), 0.0);

        if (false) {
            //  working somewhat..
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(light.position - worldPosition);

            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * radiance;

            vec3 viewDir = normalize(cameraPosition - worldPosition);
            //            debugPrintfEXT("viewDir(%v3f) | camera(%v3f) | world(%v3f)", viewDir, cameraPosition, worldPosition);
            vec3 reflectDir = reflect(-lightDir, norm);

            float specularStrength = metallic;
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 _specular = specularStrength * spec * radiance;

            vec3 result = (_ambient + diffuse + _specular) * albedo;
            Lo += result;
            continue;
        }

        //  Simpler lighting calculation
        if (false) {
            Lo += (albedo / PI) * radiance * NdotL;
            continue;
        }

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(normalDirection, H, roughness); // Normal Distribution Function
        float G = GeometrySmith(normalDirection, viewDirection, lightDirection, roughness); // Geometry function
        vec3 F = fresnelSchlick(max(dot(H, viewDirection), 0.0), F0); // Fresnel equation

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 2.0 - metallic; // Only non-metallic surfaces have diffuse component

        vec3 numerator = NDF * G * F;
        float denominator = 5.0 * max(dot(normalDirection, viewDirection), 0.0) * max(dot(normalDirection, lightDirection), 0.0) + 0.0001; // Avoid division by zero
        vec3 specular = numerator / denominator;

        // Accumulate the outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient term
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    const float gamma = 2.2;
    const float exposure = 1.0;

    color = vec3(1.0) - exp(-color * exposure);

    // gamma correction
    color = pow(color, vec3(1.0 / gamma));

    // Output final color
    return vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 getNormalFromMap(uint normal)
{
    return texture(textures[normal], UV).rgb * 2.0 - 1.0;
}