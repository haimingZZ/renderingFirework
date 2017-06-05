#define DEBUG_GS   // Uncomment this line to debug vertex shaders 

cbuffer cb0
{	
    float g_fGlobalTime;
    float g_fElapsedTime;
    float g_fSecondsPerFirework = 1.0;
    int g_iNumEmber1s = 30;
	float4 g_vFrameGravity;
    float g_fMaxEmber2s = 15.0;
};

Texture1D g_txRandom;
SamplerState g_samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
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
// Sample a random direction from our random texture
//
float3 RandomDir(float fOffset)
{
    float tCoord = (g_fGlobalTime + fOffset) / 300.0;
    return g_txRandom.SampleLevel( g_samPoint, tCoord, 0 );
}

struct VSParticleIn
{
    float3 pos              : POSITION;         //position of the particle
	float  Timer            : TIMER;            //timer for the particle
    float3 vel              : NORMAL;           //velocity of the particle
    uint   Type             : TYPE;             //particle type
};

//
// Generic particle motion handler
//

void GSGenericHandler( VSParticleIn input, inout PointStream<VSParticleIn> ParticleOutputStream )
{
    input.pos += input.vel*g_fElapsedTime;
    input.vel += g_vFrameGravity;
    input.Timer -= g_fElapsedTime;
    ParticleOutputStream.Append( input );
}

//
// Launcher type particle handler
//
void GSLauncherHandler( VSParticleIn input, inout PointStream<VSParticleIn> ParticleOutputStream )
{
     if(input.Timer <= 0)
     {
        float3 vRandom = normalize( RandomDir( input.Type ) );
        //time to emit a new SHELL
        VSParticleIn output;
        output.pos = input.pos + input.vel*g_fElapsedTime;
        output.vel = input.vel + vRandom*8.0;
        output.Timer = P_SHELLLIFE + vRandom.y*0.5;
        output.Type = PT_SHELL;
        ParticleOutputStream.Append( output );
        
        //reset our timer
        input.Timer = g_fSecondsPerFirework/* + vRandom.x*0.4*/;
    }
    else
    {
        input.Timer -= g_fElapsedTime;
    }
    
    //emit ourselves to keep us alive
    ParticleOutputStream.Append( input );
}

//
// Shell type particle handler
//
void GSShellHandler( VSParticleIn input, inout PointStream<VSParticleIn> ParticleOutputStream )
{
    if(input.Timer <= 0)
    {
        VSParticleIn output;
        float3 vRandom = float3(0,0,0);
        
        //time to emit a series of new Ember1s  
        for(int i=0; i<g_iNumEmber1s; i++)
        {
            vRandom = normalize( RandomDir( input.Type + i ) );
            output.pos = input.pos + input.vel*g_fElapsedTime;
            output.vel = input.vel + vRandom*15.0;
            output.Timer = P_EMBER1LIFE;
            output.Type = PT_EMBER1;
            ParticleOutputStream.Append( output );
        }
        
        //find out how many Ember2s to emit
        for(int i=0; i<abs(vRandom.x)*g_fMaxEmber2s; i++)
        {
            vRandom = normalize( RandomDir( input.Type + i ) );
            output.pos = input.pos + input.vel*g_fElapsedTime;
            output.vel = input.vel + vRandom*10.0;
            output.Timer = P_EMBER2LIFE + 0.4*vRandom.x;
            output.Type = PT_EMBER2;
            ParticleOutputStream.Append( output );
        }
        
    }
    else
    {
        GSGenericHandler( input, ParticleOutputStream );
    }
}

//
// Ember1 and Ember3 type particle handler
//
void GSEmber1Handler( VSParticleIn input, inout PointStream<VSParticleIn> ParticleOutputStream )
{
    if(input.Timer > 0)
    {
        GSGenericHandler( input, ParticleOutputStream );
    }
}

//
// Ember2 type particle handler
//
void GSEmber2Handler( VSParticleIn input, inout PointStream<VSParticleIn> ParticleOutputStream )
{
    if(input.Timer <= 0)
    {
        VSParticleIn output;
    
        //time to emit a series of new Ember3s  
        for(int i=0; i<10; i++)
        {
            output.pos = input.pos + input.vel*g_fElapsedTime;
            output.vel = input.vel + normalize( RandomDir( input.Type + i ) )*10.0;
            output.Timer = P_EMBER3LIFE;
            output.Type = PT_EMBER3;
            ParticleOutputStream.Append( output );
        }
    }
    else
    {
        GSGenericHandler( input, ParticleOutputStream );
    }
}

//
// Main particle system handler... handler particles and streams them out to a vertex buffer
//
[maxvertexcount(128)]
void GSAdvanceParticlesMain(point VSParticleIn input[1], inout PointStream<VSParticleIn> ParticleOutputStream)
{
	if( input[0].Type == PT_LAUNCHER )
        GSLauncherHandler( input[0], ParticleOutputStream );
    else if ( input[0].Type == PT_SHELL )
        GSShellHandler( input[0], ParticleOutputStream );
    else if ( input[0].Type == PT_EMBER1 ||
              input[0].Type == PT_EMBER3 )
        GSEmber1Handler( input[0], ParticleOutputStream );
    else if( input[0].Type == PT_EMBER2 )
        GSEmber2Handler( input[0], ParticleOutputStream );
}