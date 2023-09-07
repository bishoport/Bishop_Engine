#version 460 core




struct DirLight {
    vec3 direction;
    vec3 lightPos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool blinn;

    //SHADOWS
    float shadowIntensity;
    int usePoisonDisk;
};


struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


struct Material
{
    int       hasDiffuseMap;
    sampler2D diffuseMap;
    vec3      diffuseColor;

    sampler2D specularMap;
    float     normalIntensity;

    sampler2D normalMap;
    float     shininess;
};


uniform vec3 globalAmbient;
uniform DirLight dirLight;
uniform PointLight pointLights[16];
uniform int numPointLights;
uniform Material material;
uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform mat4 shadowBiasMVP;



//ENTRADAS
in vec3 FragPos;
in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;


//SALIDA
out vec4 FragColor;


const vec2 poissonDisk[64] = vec2[](
    vec2(-0.613392, 0.617481), vec2(0.170019, -0.040254), vec2(-0.299417, 0.791925), vec2(0.645680, 0.493210),
    vec2(-0.651784, 0.717887), vec2(0.421003, 0.027070), vec2(-0.817194, -0.271096), vec2(-0.705374, -0.668203),
    vec2(0.977050, -0.108615), vec2(0.063326, 0.142369), vec2(0.203528, 0.214331), vec2(-0.667531, 0.326090),
    vec2(-0.098422, -0.295755), vec2(-0.885922, 0.215369), vec2(0.566637, 0.605213), vec2(0.039766, -0.396100),
    vec2(0.751946, 0.453352), vec2(0.078707, -0.715323), vec2(-0.075838, -0.529344), vec2(0.724479, -0.580798),
    vec2(0.222999, -0.215125), vec2(-0.467574, -0.405438), vec2(-0.248268, -0.814753), vec2(0.354411, -0.887570),
    vec2(0.175817, 0.382366), vec2(0.487472, -0.063082), vec2(-0.084078, 0.898312), vec2(0.488876, -0.783441),
    vec2(0.470016, 0.217933), vec2(-0.696890, -0.549791), vec2(-0.149693, 0.605762), vec2(0.034211, 0.979980),
    vec2(0.503098, -0.308878), vec2(-0.016205, -0.872921), vec2(0.385784, -0.393902), vec2(-0.146886, -0.859249),
    vec2(0.643361, 0.164098), vec2(0.634388, -0.049471), vec2(-0.688894, 0.007843), vec2(0.464034, -0.188818),
    vec2(-0.440840, 0.137486), vec2(0.364483, 0.511704), vec2(0.034028, 0.325968), vec2(0.099094, -0.308023),
    vec2(0.693960, -0.366253), vec2(0.678884, -0.204688), vec2(0.001801, 0.780328), vec2(0.145177, -0.898984),
    vec2(0.062655, -0.611866), vec2(0.315226, -0.604297), vec2(-0.780145, 0.486251), vec2(-0.371868, 0.882138),
    vec2(0.200476, 0.494430), vec2(-0.494552, -0.711051), vec2(0.612476, 0.705252), vec2(-0.578845, -0.768792),
    vec2(-0.772454, -0.090976), vec2(0.504440, 0.372295), vec2(0.155736, 0.065157), vec2(0.391522, 0.849605),
    vec2(-0.620106, -0.328104), vec2(0.789239, -0.419965), vec2(-0.545396, 0.538133), vec2(-0.178564, -0.596057)
    );

void main()
{
    //--CALCULO DE LAS NORMALES
    vec3 normal = texture(material.normalMap, TexCoords).rgb; // obtain normal from normal map in range [0,1]
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
    normal = mix(vec3(0.0, 0.0, 1.0), normal, material.normalIntensity);

    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    //-----------------------------------------------------------------------------------------------

    //CALCULO DEL COLOR DIFUSO (SIN LUZ)
    vec3  diffuseColor;
    if (material.hasDiffuseMap == 1) {
        vec4 texColor = texture(material.diffuseMap, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
        diffuseColor = texColor.rgb * material.diffuseColor;
    }
    else {
        diffuseColor = material.diffuseColor;
    }
    //-----------------------------------------------------------------------------------------------

    vec3 color = vec3(0);
    color += diffuseColor;
    FragColor = vec4((color), 1.0);
}

