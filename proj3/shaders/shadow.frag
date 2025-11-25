#version 330 core
out vec4 FragColor;

in vec4 FragPosLightSpace;

// Use sampler2DShadow for hardware PCF (because of MakeShadowMap)
uniform sampler2DShadow u_shadowMap; 

float ShadowCalculation(vec4 fragPosLightSpace) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if outside shadow map range
    if(projCoords.z > 1.0)
        return 0.0;
        
    // Hardware PCF: The .z component is compared against the texture value.
    // texture(sampler2DShadow, vec3(uv, depth_to_compare)) returns proportion of pass/fail.
    // A small bias is often still needed for the depth comparison
    float bias = 0.005;
    float shadow = texture(u_shadowMap, vec3(projCoords.xy, projCoords.z - bias));
    
    // The result is 1.0 (lit) or 0.0 (shadow) or in between (PCF filtered)
    // We invert it because usually 0.0 in shadow calc means "in shadow"
    return 1.0 - shadow; 
}

void main() {
    // ... Calculate Ambient, Diffuse, Specular ...
    
    float shadow = ShadowCalculation(FragPosLightSpace);
    
    // Apply shadow to Diffuse and Specular only (Ambient is always present)
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    
    FragColor = vec4(lighting, 1.0);
}