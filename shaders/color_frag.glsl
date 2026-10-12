#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

// lighting information
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

// camera information
uniform vec3 uViewPos;

// object information
uniform vec3 uColor;

// coefficient
uniform float ka;
uniform float kd;
uniform float ks;
uniform float alpha;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    vec3 ambient = ka * uAmbientColor;
    vec3 diffuse = kd * max(dot(N, L), 0.0) * uLightColor;
    vec3 specular = ks * uLightColor * pow(max(dot(N, H), 0.0), alpha);

    vec3 color = ambient + diffuse + specular;

    fragColor = vec4(color * uColor, 1.0);
}
