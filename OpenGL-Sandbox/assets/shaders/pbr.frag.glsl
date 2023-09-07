#version 330 core
//#INCLUDE_COMMON

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform Material material;

uniform vec3 globalAmbient;

// lights
uniform PointLight pointLights[16];
uniform int numPointLights;


uniform vec3 camPos;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anyways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()  //material.normalIntensity
{
    float intensity = material.normalIntensity; // Define una variable para la intensidad
    vec3 tangentNormal = texture(material.normalMap, TexCoords).xyz * 2.0 - 1.0;

    // Modifica la fuerza del mapa normal en el espacio tangencial
    //tangentNormal *= vec3(1.0, 1.0, intensity);

    vec3 Q1 = dFdx(WorldPos);
    vec3 Q2 = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}



// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------





void main()
{		
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    if (material.hasAlbedoMap)
        albedo = pow(texture(material.albedoMap, TexCoords).rgb, vec3(1.0));
    else
        albedo = material.albedoColor;

    if (material.hasMetallicMap)
        metallic = texture(material.metallicMap, TexCoords).r * material.metallicValue;
    else
        metallic = material.metallicValue;

    if (material.hasRougnessMap)
        roughness = texture(material.roughnessMap, TexCoords).r * material.roughnessValue;
    else
        roughness = material.roughnessValue;

    if (material.hasAoMap)
        ao = texture(material.aoMap, TexCoords).r;
    else
        ao = 1.0;



    vec3 N = getNormalFromMap();      
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(material.reflectanceValue);
    F0 = mix(F0, albedo, metallic);

    //reflectance equation
    vec3 Lo = vec3(0.0);



    for (int i = 0; i < numPointLights; ++i) 
    {
        if (pointLights[i].isActive == false)
            continue;

        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - WorldPos);

        if (distance > pointLights[i].range)
            continue;

        float distanceRatio = distance / pointLights[i].range;
        float threshold1 = 1.0 - pointLights[i].lightSmoothness;  // Start of the transition
        float threshold2 = 1.0;  // End of the transition
        float fadeFactor = 1.0 - smoothstep(threshold1, threshold2, distanceRatio);
        float attenuation = fadeFactor / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));
        //float attenuation = 1.0 / (distance * distance);
        vec3 radiance = (pointLights[i].ambient * pointLights[i].strength)  * attenuation;


        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);  

        //SHADOWS
        if (pointLights[i].drawShadows == true)
        {
            // get vector between fragment position and light position
            vec3 fragToLight = WorldPos - pointLights[i].position;
            // ise the fragment to light vector to sample from the depth map    
            float closestDepth = texture(pointLights[i].shadowMap, fragToLight).r;
            // it is currently in linear range between [0,1], let's re-transform it back to original depth value
            closestDepth *= pointLights[i].range;
            // now get current linear depth as the length between the fragment and light position
            float currentDepth = length(fragToLight);
            // test for shadows
            float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
            float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

            Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
        }
        else
        {
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
    }   
    

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color , 1.0);
}