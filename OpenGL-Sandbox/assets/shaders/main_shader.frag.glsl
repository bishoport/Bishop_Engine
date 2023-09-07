#version 460 core
//#INCLUDE_COMMON

uniform vec3 globalAmbient;
uniform DirLight dirLight;
uniform bool useDirLight;

uniform PointLight pointLights[16];
uniform int numPointLights;

uniform Material material;
uniform vec3 viewPos;


//ENTRADAS
in vec3 FragPos;
in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

//SALIDA
out vec4 FragColor;








void main()
{
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 color = vec3(0);

    //--NORMAL MAP
    vec3 normal;
    if (material.hasNormalMap == true) {
        normal = texture(material.normalMap, TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = mix(vec3(0.0, 0.0, 1.0), normal, material.normalIntensity);
    }
    else {
        normal = vec3(0.0, 0.0, 1.0); // Just use the default upwards-pointing normal.
    }


    //--DIFFUSE COLOR
    vec3 diffuseColor;
    if (material.hasDiffuseMap == true) {
        vec4 texColor = texture(material.diffuseMap, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
        diffuseColor = texColor.rgb * material.diffuseColor;
    }
    else {
        diffuseColor = material.diffuseColor;
    }
    //----------------------------------------------------------------------


    //--SPECULAR COLOR
    vec3 specularValue = vec3(1.0); // Default white if no texture
    if (material.hasSpecularMap == true) 
    {
        specularValue = texture(material.specularMap, TexCoords).rgb;
    }
    else
    {
        if (material.hasNormalMap == true)
        {
            vec3 normalFromMap = normalize(texture(material.normalMap, TexCoords).rgb * 2.0 - 1.0);
            float roughness = length(normalFromMap - normal);
            specularValue = mix(vec3(1.0), vec3(0.5), roughness); // Aquí estamos interpolando entre un brillo completo y una reducción del 50% basada en la rugosidad.
        }
        else if (material.hasDiffuseMap == true)
        {
            vec3 diff = texture(material.diffuseMap, TexCoords).rgb;
            specularValue = diff * 0.5; // This is just an example multiplier
        }
    }
    //----------------------------------------------------------------------



    //--DIRECTIONAL LIGHT
    float direrectLight_shadowResult = 1.0;
    if (useDirLight == true)
    {
        if (dirLight.isActive == true)
        {
            //__Color
            float diffDir = max(dot(lightDir, normal), 0.0);
            color += dirLight.diffuse * diffDir * diffuseColor;

            //__Specular
            float spec = 0.0;
            if (dirLight.blinn)
            {
                vec3 halfwayDir = normalize(lightDir + viewDir);
                spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
            }
            else
            {
                vec3 reflectDir = reflect(-lightDir, normal);
                spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
            }
            color += dirLight.specular * spec * specularValue * dirLight.specularPower;
            //-----------------------------------------------------------------------------------------------


            //--SHADOWS DIR LIGHT
            if (dirLight.drawShadows == true)
            {
                vec4 shadowCoord = dirLight.shadowBiasMVP * vec4(FragPos, 1.0);;
                vec3 sCoord = shadowCoord.xyz / shadowCoord.w;
                float bias = 0.005 * tan(acos(dot(normal, lightDir)));
                if (dirLight.usePoisonDisk == 1)
                {
                    for (int j = 0; j < 64; j++) {
                        if (texture(dirLight.shadowMap, sCoord.xy + poissonDisk[j] / 300.0).r < sCoord.z - bias)
                            direrectLight_shadowResult -= dirLight.shadowIntensity / 64.;
                    }
                }
                else
                {
                    if (texture(dirLight.shadowMap, sCoord.xy).r < sCoord.z - bias)
                        direrectLight_shadowResult -= dirLight.shadowIntensity;
                }
            }
        }
    }
    
    //-----------------------------------------------------------------------------------------------




    //--POINT LIGHTS
    float pointsLight_shadowResult = 1.0;
    for (int i = 0; i < numPointLights; i++)
    {
        if (pointLights[i].isActive == false)
            continue;

        float distance = length(FragPos - pointLights[i].position);

        if (distance > pointLights[i].range)
            continue;

        float distanceRatio = distance / pointLights[i].range;
        float threshold1 = 1.0 - pointLights[i].lightSmoothness;  // Start of the transition
        float threshold2 = 1.0;  // End of the transition
        float fadeFactor = 1.0 - smoothstep(threshold1, threshold2, distanceRatio);

        float diffPoint = max(dot(lightDir, normal), 0.0);

        //__Specular
        float spec = 0.0;
        if (pointLights[i].blinn)
        {
            vec3 halfwayDir = normalize(lightDir + viewDir);
            spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        }
        else
        {
            vec3 reflectDir = reflect(-lightDir, normal);
            spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
        }
        //-----------------------------------------------------------------------------------------------


        float attenuation = fadeFactor / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));



        //SHADOWS
        if (pointLights[i].drawShadows == true)
        {
            // get vector between fragment position and light position
            vec3 fragToLight = FragPos - pointLights[i].position;
            // ise the fragment to light vector to sample from the depth map    
            float closestDepth = texture(pointLights[i].shadowMap, fragToLight).r;
            // it is currently in linear range between [0,1], let's re-transform it back to original depth value
            closestDepth *= pointLights[i].range;
            // now get current linear depth as the length between the fragment and light position
            float currentDepth = length(fragToLight);
            // test for shadows
            float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
            float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
            // display closestDepth as debug (to visualize depth cubemap)
            // FragColor = vec4(vec3(closestDepth / far_plane), 1.0); 

            color += pointLights[i].strength * ((pointLights[i].ambient + (1.0 - shadow)) * diffuseColor
                + pointLights[i].diffuse * diffPoint * diffuseColor
                + pointLights[i].specular * spec * specularValue * pointLights[i].specularPower)
                * attenuation * (1.0 - shadow);
        }
        else
        {
            color += pointLights[i].strength * (pointLights[i].ambient * diffuseColor
                + pointLights[i].diffuse * diffPoint * diffuseColor
                + pointLights[i].specular * spec * specularValue * pointLights[i].specularPower)
                * attenuation;
        }
    }
    //-----------------------------------------------------------------------------------------------


    //--AMBIENT LIGHT
    color += globalAmbient * diffuseColor;
    //-----------------------------------------------------------------------------------------------



    FragColor = vec4(color * direrectLight_shadowResult, 1.0);
}