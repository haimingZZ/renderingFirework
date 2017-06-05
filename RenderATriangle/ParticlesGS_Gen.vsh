#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
struct VSParticleIn
{
    float3 pos              : POSITION;         //position of the particle
	float  Timer            : TEXCOORD0;            //timer for the particle
    float3 vel              : NORMAL;           //velocity of the particle
    uint   Type             : TEXCOORD1;             //particle type
};

//
// Passthrough VS for the streamout GS
//
VSParticleIn VSPassThroughmain(VSParticleIn input)
{
    return input;
}