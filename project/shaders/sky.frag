#version 330 core

uniform sampler2D Tex;
uniform sampler2D glow;
uniform sampler2D colorSky;

in vec3 lDir;
in vec3 normals;
in vec3 position;
in vec2 UV;

out vec4 color;

void main()
{
    vec3 V = position;
    vec3 L = lDir;
    vec3 N = normals;

    // Compute the proximity of this fragment to the sun.
    float dotVL = dot(V, L);

    // Look up the sky color and glow colors.
    //vec4 Kc = texture(colorSky, vec2((L.y + 1.0) / 2.0, V.y));
    //vec4 Kg = texture(glow,  vec2((L.y + 1.0) / 2.0, dotVL));

    vec4 sTexColor = texture(Tex, UV.st);

    // Combine the color and glow giving the pixel value.
    //vec3 shadedcolor = Kc.rgb * dotNL + Kg.rgb * Kg.a / 2.0;
    //color =  vec4( shadedcolor , Kc.a);

    vec3 A = vec3(0.3f, 0.4f, 0.8f);
    vec3 D = vec3(0.8f, 0.8f, 1.0f);
    vec3 S = vec3(1.0f, 1.0f, 1.0f);

    vec3 R = 2.0* dot (N ,L) *L - L; // Could also have used the function reflect ()
    float dotNL = max ( dot (N , L) , 0.0) ; // If negative , set to zero
    float dotRV = max ( dot (R , V) , 0.0) ;
    if ( dotNL == 0.0) dotRV = 0.0; // Do not show highlight on the dark side
    vec3 shadedcolor = A + D * dotNL + S * dotRV;
    color =  sTexColor * vec4( shadedcolor , 1.0) ;



}
