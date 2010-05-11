[VertexShader]
//
// Vertex shader for adaptively antialiasing a procedural stripe pattern
//
// Author: Randi Rost
//         based on a shader by Bert Freudenberg
//
// Copyright (c) 2002-2005 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

uniform vec3  LightPosition;

varying float V;
varying float LightIntensity;
 
void main()
{
    vec3 pos        = vec3(gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - pos);

    LightIntensity = max(dot(lightVec, tnorm), 0.0);

    V = gl_MultiTexCoord0.s;  // try .s for vertical stripes

    gl_Position = ftransform();
}
[FragmentShader]
//
// Fragment shader for adaptively antialiasing a procedural stripe pattern
//
// Author: Randi Rost
//         based on a shader by Bert Freudenberg
//
// Copyright (c) 2002-2005 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

varying float V;                    // generic varying
varying float LightIntensity;

uniform float Frequency;            // Stripe frequency = 16

void main()
{
    float sawtooth = fract(V * Frequency);
    float triangle = abs(2.0 * sawtooth - 1.0);
    float dp = length(vec2(dFdx(V), dFdy(V)));
    float edge = dp * Frequency * 2.0;
    float square = smoothstep(0.5 - edge, 0.5 + edge, triangle);
    gl_FragColor = vec4(vec3(square), 1.0) * LightIntensity;
}
[Parameters]
float Frequency = 8;
vec3 LightPosition = vec3(0, 0, 4);
