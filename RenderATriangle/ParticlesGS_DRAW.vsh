#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
struct VSParticleIn
{
    float3 pos              : POSITION;         //position of the particle
	float  Timer            : TEXCOORD0;            //timer for the particle
    float3 vel              : NORMAL;           //velocity of the particle
    uint   Type             : TEXCOORD1;             //particle type
};

struct VSParticleDrawOut
{
    float3 pos : POSITION;
    float4 color : COLOR0;
    float radius : RADIUS;
};

//
// Explanation of different particle types
//
#define PT_LAUNCHER 0 //Firework Launcher - launches a PT_SHELL every so many seconds
#define PT_SHELL    1 //Unexploded shell - flies from the origin and explodes into many PT_EMBERXs
#define PT_EMBER1   2 //basic particle - after it's emitted from the shell, it dies
#define PT_EMBER2   3 //after it's emitted, it explodes again into many PT_EMBER1s
#define PT_EMBER3   4 //just a differently colored ember1
#define P_SHELLLIFE 3.0
#define P_EMBER1LIFE 2.5
#define P_EMBER2LIFE 1.5
#define P_EMBER3LIFE 2.0

//
// Vertex shader for drawing the point-sprite particles
//
VSParticleDrawOut VSScenemain(VSParticleIn input)
{
    VSParticleDrawOut output = (VSParticleDrawOut)0;
    
    //
    // Pass the point through
    //
    output.pos = input.pos;
    output.radius = 1.5;
    
    //  
    // calculate the color
    //
    if( input.Type == PT_LAUNCHER )
    {
        output.color = float4(1,0.1,0.1,1);
        output.radius = 1.0;
    }
    else if( input.Type == PT_SHELL )
    {
        output.color = float4(0.1,1,1,1);
        output.radius = 1.0;
    }
    else if( input.Type == PT_EMBER1 )
    {
        output.color = float4(1,1,0.1,1);
        output.color *= (input.Timer / P_EMBER1LIFE );
    }
    else if( input.Type == PT_EMBER2 )
    {
        output.color = float4(1,0.1,1,1);
    }
    else if( input.Type == PT_EMBER3 )
    {
        output.color = float4(1,0.1,0.1,1);
        output.color *= (input.Timer / P_EMBER3LIFE );
    }
    
    return output;
}