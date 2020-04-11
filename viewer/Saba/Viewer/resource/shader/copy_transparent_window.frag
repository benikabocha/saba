#version 140

in vec2 vs_UV;

uniform sampler2D u_Tex;

out vec4 fs_Color;

void main()
{
    vec4 texColor = texture(u_Tex, vs_UV);
    float invAlpha = 1.0 - texColor.a;
    if (invAlpha == 0.0)
    {
        fs_Color = vec4(1.0, 0.0, 1.0, 0.0);
    }
    else
    {
        fs_Color.rgb = texColor.rgb;
        fs_Color.a = invAlpha;
    }
}
