#include <windows.h>
#include <d3d11.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <xnamath.h>
#include "resource.h"
#include <fstream>
#include "ConstBuffer.h"
#include <fstream>
using namespace std;


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

/***************************************************************************************/
//与Directx初始化相关的变量;
/***************************************************************************************/
HINSTANCE   g_hInst		= NULL;
HWND        g_hWnd		= NULL;
int			g_Width		= 0;
int         g_Height	= 0;

ID3D11Device*			g_pD3DDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL;
IDXGISwapChain*			g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

ID3D11Texture2D*		g_pDepthStencil	= NULL;
ID3D11DepthStencilView*	g_pDepthStencilView = NULL;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
//所有的程序都需要一个窗口，用InitWindow来封装创建窗口的过程
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow, int width, int height);
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
//每一个DX11的程序一开始都需要创建三个对象：ID3D11Device，ID3D11DeviceContext，IDXGISwapChain
//用Renderer表示一个底层渲染的借口，InitRenderer来初始化渲染
HRESULT InitRnederer();
//完成渲染结束时的清除工作;
void	ClearRenderer();
//执行渲染工作;
void    Render(float fTime, float deltaTime);

//初始化场景;
HRESULT InitScene();
//清除场景;
void	ClearScene();
//unsigned int g_program_start_time = 0;
//DWORD flasttime;
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	if( FAILED(InitWindow(hInstance, nCmdShow, 640, 480)))
		return 0;

	if (FAILED(InitRnederer()))
	{
		ClearRenderer();
		return 0;
	}

	if (FAILED(InitScene()))
	{
		ClearScene();
		ClearRenderer();
		return 0;
	}

	// Main message loop
	MSG msg = {0};
	DWORD lasttime = GetTickCount();
	unsigned int g_program_start_time = GetTickCount();
	//flasttime = GetTickCount();
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			float delta = 0.0f;
			DWORD curtime = GetTickCount();
			delta = (float)(curtime - lasttime) / 1000.0f;
			float fTime = (float)(curtime - g_program_start_time) / 1000.0f;
			Render(fTime, delta);
			lasttime = curtime;
			g_pSwapChain->Present(0, 0);
		}
	}

	ClearRenderer();
	return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow, int width, int height)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_DIRECTX11 );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "Particles";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_DIRECTX11 );
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	g_Width = width;
	g_Height= height;
	RECT rc = { 0, 0, width, height};
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	g_hWnd = CreateWindow( "Particles", "Particles", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL );
	if( !g_hWnd )
		return E_FAIL;

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message )
	{
	case WM_PAINT:
		hdc = BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		break;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}

//每一个DX11的程序一开始都需要创建三个对象：ID3D11Device，ID3D11DeviceContext，IDXGISwapChain
//用Renderer表示一个底层渲染的借口，InitRenderer来初始化渲染
HRESULT InitRnederer()
{
	//创建swap chain;
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_Width;
	sd.BufferDesc.Height= g_Height;
	sd.BufferDesc.Format= DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, NULL, 0, 
		D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pD3DDevice, NULL, &g_pImmediateContext)))
	{
		return FALSE;
	}

	//创建一个render target view;
	HRESULT hr;
	ID3D11Texture2D* pBackBuffer;
	if(FAILED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)))
		return FALSE;
	hr = g_pD3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if(FAILED(hr))
		return FALSE;

	//Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = g_Width;
	descDepth.Height= g_Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format	= DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pD3DDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;
	//Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pD3DDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


	//设置光栅化状态;
	D3D11_RASTERIZER_DESC rasterizerState;
	rasterizerState.CullMode = D3D11_CULL_BACK;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.FrontCounterClockwise = false;
	rasterizerState.DepthBias = false;
	rasterizerState.DepthBiasClamp = 0;
	rasterizerState.SlopeScaledDepthBias = 0;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.ScissorEnable = false;
	rasterizerState.MultisampleEnable = false;
	rasterizerState.AntialiasedLineEnable = true;

	ID3D11RasterizerState* pRS;
	g_pD3DDevice->CreateRasterizerState(&rasterizerState, &pRS);
	g_pImmediateContext->RSSetState(pRS);


	D3D11_DEPTH_STENCIL_DESC  depth_stencil_desc;
	ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
	depth_stencil_desc.DepthEnable = true;
	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	ID3D11DepthStencilState* g_pDepthStencilState;
	g_pD3DDevice->CreateDepthStencilState(&depth_stencil_desc, &g_pDepthStencilState);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)g_Width;
	vp.Height = (FLOAT)g_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports( 1, &vp );
	return S_OK;
}
//完成渲染结束时的清除工作;
void	ClearRenderer()
{
	if(g_pImmediateContext) g_pImmediateContext->ClearState();

	if(g_pDepthStencilView) g_pDepthStencilView->Release();
	if(g_pDepthStencil) g_pDepthStencil->Release();
	if(g_pRenderTargetView) g_pRenderTargetView->Release();
	if(g_pSwapChain) g_pSwapChain->Release();
	if(g_pImmediateContext) g_pImmediateContext->Release();
	if(g_pD3DDevice) g_pD3DDevice->Release();
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( CHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
	if( FAILED(hr) )
	{
		if( pErrorBlob != NULL )
		{
			MessageBox(NULL, (char*)pErrorBlob->GetBufferPointer(), "", MB_OK);
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
		}
		if( pErrorBlob ) pErrorBlob->Release();
		return hr;
	}
	if( pErrorBlob ) pErrorBlob->Release();

	return S_OK;
}



/***************************************************************************************/
//场景相关变量;
/***************************************************************************************/
XMMATRIX				g_World;
XMMATRIX				g_View;
XMMATRIX				g_Projection;
//粒子系统生成shader的相关变量;
ID3D11VertexShader*		g_pParticlesGenVS = NULL;
ID3D11GeometryShader*	g_pParticlesGenGS = NULL;

ID3D11Buffer*			g_pLauncherBuffer = NULL;
ID3D11Buffer*			g_pParticleDrawFrom = NULL;
ID3D11Buffer*			g_pParticleStreamTo = NULL;

ID3D11InputLayout*		g_pGenInputLayout = NULL;

VariableInfo g_TxRadomInfo;
VariableInfo g_SamPointInfo;
ID3D11ShaderResourceView*	g_pTxRadom			= NULL;
ID3D11SamplerState*			g_pSamPoint			= NULL;

#define GEN_GS_CONSTBUFFER_SIZE 1

vector<ConstantBufferInfo*> g_GenGSConstBufferInfo;
vector<ID3D11Buffer*>		g_GenGSConstantBufferBlob;


//粒子系统绘制shader相关的变量;
ID3D11VertexShader*		g_pParticlesDrawVS = NULL;
ID3D11GeometryShader*	g_pParticlesDrawGS = NULL;
ID3D11PixelShader*		g_pParitclesDrawPS = NULL;

ID3D11InputLayout*		g_pDrawInputLayout = NULL;

#define DRAW_GS_CONSTBUFFER_SIZE 2

vector<ConstantBufferInfo*> g_DrawGSConstBufferInfo;
vector<ID3D11Buffer*>		g_DrawGSConstantBufferBlob;

VariableInfo g_txDiffuseInfo;
VariableInfo g_samLinearInfo;
ID3D11ShaderResourceView*	g_txDiffuse			= NULL;
ID3D11SamplerState*			g_samLinear			= NULL;

struct PARTICLE_VERTEX
{
	XMFLOAT3 pos;
	float Timer;
	XMFLOAT3 vel;
	UINT Type;
};
#define MAX_PARTICLES 30000

//--------------------------------------------------------------------------------------
// This helper function creates a 1D texture full of random vectors.  The shader uses
// the current time value to index into this texture to get a random vector.
//--------------------------------------------------------------------------------------
HRESULT CreateRandomTexture( ID3D11Device* pd3dDevice )
{
	HRESULT hr = S_OK;

	int iNumRandValues = 1024;
	srand( timeGetTime() );
	//create the data
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = new float[iNumRandValues * 4];
	if( !InitData.pSysMem )
		return E_OUTOFMEMORY;
	InitData.SysMemPitch = iNumRandValues * 4 * sizeof( float );
	InitData.SysMemSlicePitch = iNumRandValues * 4 * sizeof( float );
	for( int i = 0; i < iNumRandValues * 4; i++ )
	{
		( ( float* )InitData.pSysMem )[i] = float( ( rand() % 10000 ) - 5000 );
	}

	ID3D11Texture1D* pRandomTexture;
	// Create the texture
	D3D11_TEXTURE1D_DESC dstex;
	dstex.Width = iNumRandValues;
	dstex.MipLevels = 1;
	dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	dstex.Usage = D3D11_USAGE_DEFAULT;
	dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dstex.CPUAccessFlags = 0;
	dstex.MiscFlags = 0;
	dstex.ArraySize = 1;
	hr = pd3dDevice->CreateTexture1D( &dstex, &InitData, &pRandomTexture );
	delete [] InitData.pSysMem;

	// Create the resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
	SRVDesc.Format = dstex.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	SRVDesc.Texture2D.MipLevels = dstex.MipLevels;
	hr = pd3dDevice->CreateShaderResourceView( pRandomTexture, &SRVDesc, &g_pTxRadom );
	return hr;
}

static unsigned int getComponentCount(BYTE mask)
{
	unsigned int compCount = 0;
	if (mask&1)
		++compCount;
	if (mask&2)
		++compCount;
	if (mask&4)
		++compCount;
	if (mask&8)
		++compCount;
	return compCount;
}

//初始化场景;
HRESULT InitScene()
{
	HRESULT hr;
	/*******************************************************************************************/
	//ParticlesGS_Gen Vertex Shader
	/*******************************************************************************************/
	ID3DBlob* pParticleGenVSBlob = NULL;
	hr = CompileShaderFromFile("ParticlesGS_Gen.vsh", "VSPassThroughmain", 
		"vs_4_0", &pParticleGenVSBlob);
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	hr = g_pD3DDevice->CreateVertexShader(pParticleGenVSBlob->GetBufferPointer(), 
		pParticleGenVSBlob->GetBufferSize(), NULL, &g_pParticlesGenVS);
	if (FAILED(hr))
	{
		pParticleGenVSBlob->Release();
		return hr;
	}

	//define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD",	0, DXGI_FORMAT_R32_FLOAT,		0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD",	1, DXGI_FORMAT_R32_FLOAT,		0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0
		}
	};
	UINT numElements = ARRAYSIZE(layout);

	//create the input layout
	hr = g_pD3DDevice->CreateInputLayout(layout, numElements, pParticleGenVSBlob->GetBufferPointer(),
		pParticleGenVSBlob->GetBufferSize(), &g_pGenInputLayout);
	pParticleGenVSBlob->Release();
	if (FAILED(hr))
		return hr;
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	/************************************************************************/
	// ParticlesGS_Gen Geometry Shader   
	/************************************************************************/

	ID3DBlob* pParticlesGenGSBlob = NULL;
	hr = CompileShaderFromFile("ParticlesGS_Gen.gsh", "GSAdvanceParticlesMain", 
		"gs_4_0", &pParticlesGenGSBlob);
	if (FAILED(hr))
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}

	ID3D11ShaderReflection* pParticlesGenGSReflector = NULL;

	hr = D3DReflect(pParticlesGenGSBlob->GetBufferPointer(), pParticlesGenGSBlob->GetBufferSize(), IID_ID3D11ShaderReflection,
		(void**)&pParticlesGenGSReflector);

	if(FAILED(hr))
	{
		pParticlesGenGSBlob->Release();
		if(pParticlesGenGSReflector != NULL)
			pParticlesGenGSReflector->Release();
		return hr;
	}

	D3D11_SHADER_DESC gs_desc;
	pParticlesGenGSReflector->GetDesc(&gs_desc);
	
	D3D11_SO_DECLARATION_ENTRY* so_decl_entry = new D3D11_SO_DECLARATION_ENTRY[gs_desc.OutputParameters];
	int totalComp = 0;
	for (int i = 0; i < gs_desc.OutputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC output_desc;
		pParticlesGenGSReflector->GetOutputParameterDesc(i, &output_desc);
		so_decl_entry[i].Stream			= 0;
		so_decl_entry[i].SemanticName	= output_desc.SemanticName;
		so_decl_entry[i].SemanticIndex	= output_desc.SemanticIndex;
		so_decl_entry[i].StartComponent	= 0;
		so_decl_entry[i].ComponentCount	= getComponentCount(output_desc.Mask);
		so_decl_entry[i].OutputSlot		= 0;

		totalComp += so_decl_entry[i].ComponentCount;
	}

	unsigned int stride = sizeof(PARTICLE_VERTEX);//totalComp * sizeof(float);//;//totalComp * sizeof(float);
	hr = g_pD3DDevice->CreateGeometryShaderWithStreamOutput(pParticlesGenGSBlob->GetBufferPointer(),
		pParticlesGenGSBlob->GetBufferSize(), so_decl_entry, gs_desc.OutputParameters, &stride,
		1, D3D11_SO_NO_RASTERIZED_STREAM ,
		0, &g_pParticlesGenGS);

	delete [] so_decl_entry;
	
	if (FAILED(hr))
	{
		pParticlesGenGSBlob->Release();
		if (g_pParticlesGenGS)
			g_pParticlesGenGS->Release();
		return hr;
	}


	int gen_gs_desc_bufferC = 0;
	for (UINT k = 0; k < gs_desc.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pParticlesGenGSReflector->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_CBUFFER)
		{
			D3D11_SHADER_BUFFER_DESC shader_buffer_desc;
			ID3D11ShaderReflectionConstantBuffer* pConstBuffer = 
				pParticlesGenGSReflector->GetConstantBufferByName(input_bind_desc.Name);
			pConstBuffer->GetDesc(&shader_buffer_desc);

			ConstantBufferInfo* bufferInfo = new ConstantBufferInfo(shader_buffer_desc.Size);
			bufferInfo->slot = input_bind_desc.BindPoint;
			bufferInfo->name = shader_buffer_desc.Name;
			bufferInfo->numOfVariables = shader_buffer_desc.Variables;
			bufferInfo->size = shader_buffer_desc.Size;

			//load the description of each variable for use later on when binding a buffer
			for (UINT j = 0; j < shader_buffer_desc.Variables; j++)
			{
				VariableInfo variableInfo;
				//get the variable description and store it
				ID3D11ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC var_desc;
				pVariable->GetDesc(&var_desc);

				variableInfo.size = var_desc.Size;
				variableInfo.name = var_desc.Name;
				variableInfo.offset = var_desc.StartOffset;

				bufferInfo->variables.push_back(variableInfo);
			}
			g_GenGSConstBufferInfo.push_back(bufferInfo);
			g_GenGSConstantBufferBlob.push_back(NULL);
		}
		else if (input_bind_desc.Type == D3D_SIT_TEXTURE)
		{
			g_TxRadomInfo.slot = input_bind_desc.BindPoint;	
			g_TxRadomInfo.name = input_bind_desc.Name;
		}
		else if (input_bind_desc.Type == D3D_SIT_SAMPLER)
		{
			g_SamPointInfo.slot = input_bind_desc.BindPoint;
			g_SamPointInfo.name = input_bind_desc.Name;
		}
	}

	pParticlesGenGSReflector->Release();
	pParticlesGenGSBlob->Release();
	/////////////////////////////////////////////////////////////////////////////////

	/*******************************************************************************/
	// ParticlesGS_DRAW Vertex Shader
	/*******************************************************************************/
	ID3DBlob* pParticleDrawVSBlob = NULL;
	hr = CompileShaderFromFile("ParticlesGS_DRAW.vsh", "VSScenemain", 
		"vs_4_0", &pParticleDrawVSBlob);
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	hr = g_pD3DDevice->CreateVertexShader(pParticleDrawVSBlob->GetBufferPointer(), 
		pParticleDrawVSBlob->GetBufferSize(), NULL, &g_pParticlesDrawVS);
	if (FAILED(hr))
	{
		pParticleDrawVSBlob->Release();
		return hr;
	}

	//create the input layout
	hr = g_pD3DDevice->CreateInputLayout(layout, numElements, pParticleDrawVSBlob->GetBufferPointer(),
		pParticleDrawVSBlob->GetBufferSize(), &g_pDrawInputLayout);
	pParticleDrawVSBlob->Release();
	if (FAILED(hr))
		return hr;

	/*******************************************************************************/
	// ParticlesGS_DRAW Geometry Shader
	/*******************************************************************************/
	ID3DBlob* pParticlesDrawGSBlob = NULL;
	hr = CompileShaderFromFile("ParticlesGS_DRAW.gsh", "GSScenemain", 
		"gs_4_0", &pParticlesDrawGSBlob);
	if (FAILED(hr))
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	//create the geometry shader
	hr = g_pD3DDevice->CreateGeometryShader(pParticlesDrawGSBlob->GetBufferPointer(), 
		pParticlesDrawGSBlob->GetBufferSize(), NULL, &g_pParticlesDrawGS);


	ID3D11ShaderReflection* pParticlesDrawGSReflector = NULL;

	hr = D3DReflect(pParticlesDrawGSBlob->GetBufferPointer(), pParticlesDrawGSBlob->GetBufferSize(), IID_ID3D11ShaderReflection,
		(void**)&pParticlesDrawGSReflector);

	if(FAILED(hr))
	{
		pParticlesDrawGSBlob->Release();
		if(pParticlesDrawGSReflector != NULL)
			pParticlesDrawGSReflector->Release();
		return hr;
	}

	D3D11_SHADER_DESC draw_gs_desc;
	pParticlesDrawGSReflector->GetDesc(&draw_gs_desc);

	int draw_gs_desc_bufferC = 0;
	for (UINT k = 0; k < draw_gs_desc.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pParticlesDrawGSReflector->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_CBUFFER)
		{
			D3D11_SHADER_BUFFER_DESC shader_buffer_desc;
			ID3D11ShaderReflectionConstantBuffer* pConstBuffer = 
				pParticlesDrawGSReflector->GetConstantBufferByName(input_bind_desc.Name);
			pConstBuffer->GetDesc(&shader_buffer_desc);

			ConstantBufferInfo* bufferInfo = new ConstantBufferInfo(shader_buffer_desc.Size);
			bufferInfo->slot = input_bind_desc.BindPoint;
			bufferInfo->name = shader_buffer_desc.Name;
			bufferInfo->numOfVariables = shader_buffer_desc.Variables;
			bufferInfo->size = shader_buffer_desc.Size;

			//load the description of each variable for use later on when binding a buffer
			for (UINT j = 0; j < shader_buffer_desc.Variables; j++)
			{
				VariableInfo variableInfo;
				//get the variable description and store it
				ID3D11ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC var_desc;
				pVariable->GetDesc(&var_desc);

				variableInfo.size = var_desc.Size;
				variableInfo.name = var_desc.Name;
				variableInfo.offset = var_desc.StartOffset;

				D3D11_SHADER_TYPE_DESC type_desc;
				ID3D11ShaderReflectionType* pType = pVariable->GetType();
				pType->GetDesc(&type_desc);

				bufferInfo->variables.push_back(variableInfo);
			}
			g_DrawGSConstBufferInfo.push_back(bufferInfo);
			g_DrawGSConstantBufferBlob.push_back(NULL);
		}
		
	}

	pParticlesDrawGSReflector->Release();
	pParticlesDrawGSBlob->Release();
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	// Compile the pixel shader
	ID3DBlob* pParticlesDrawPSBlob = NULL;
	hr = CompileShaderFromFile( "ParticlesGS_DRAW.psh", "PSScenemain", "ps_4_0", &pParticlesDrawPSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	// Create the pixel shader
	hr = g_pD3DDevice->CreatePixelShader( pParticlesDrawPSBlob->GetBufferPointer(),
		pParticlesDrawPSBlob->GetBufferSize(), NULL, &g_pParitclesDrawPS );

	ID3D11ShaderReflection* pParticlesDrawPSReflector = NULL;

	hr = D3DReflect(pParticlesDrawPSBlob->GetBufferPointer(), pParticlesDrawPSBlob->GetBufferSize(), 
		IID_ID3D11ShaderReflection, (void**)&pParticlesDrawPSReflector);

	if(FAILED(hr))
	{
		pParticlesDrawPSBlob->Release();
		if(pParticlesDrawPSReflector != NULL)
			pParticlesDrawPSReflector->Release();
		return hr;
	}

	D3D11_SHADER_DESC desc1;
	pParticlesDrawPSReflector->GetDesc(&desc1);

	int buffercount = 0;
	for (UINT k = 0; k < desc1.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pParticlesDrawPSReflector->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_TEXTURE)
		{
			g_txDiffuseInfo.slot = input_bind_desc.BindPoint;	
			g_txDiffuseInfo.name = input_bind_desc.Name;
		}
		else if (input_bind_desc.Type == D3D_SIT_SAMPLER)
		{
			g_samLinearInfo.slot = input_bind_desc.BindPoint;
			g_samLinearInfo.name = input_bind_desc.Name;
		}
	}

	pParticlesDrawPSReflector->Release();
	//////////////////////////////////////////////////////////////

	pParticlesDrawPSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(PARTICLE_VERTEX);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));

	PARTICLE_VERTEX vertStart =
	{
		XMFLOAT3( 0, 0, 0 ),
		float( 0 ),
		XMFLOAT3( 0, 40, 0 ),
		UINT( 0 ),
	};

	InitData.pSysMem = &vertStart;
	InitData.SysMemPitch = sizeof(PARTICLE_VERTEX );
	if(FAILED(g_pD3DDevice->CreateBuffer(&bd, &InitData, &g_pLauncherBuffer)))
		return FALSE;

	//创建ping-pong buffer;
	bd.ByteWidth = MAX_PARTICLES * sizeof( PARTICLE_VERTEX );
	bd.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
	g_pD3DDevice->CreateBuffer( &bd, NULL, &g_pParticleDrawFrom );
	g_pD3DDevice->CreateBuffer( &bd, NULL, &g_pParticleStreamTo );


	for (int i = 0; i < g_GenGSConstBufferInfo.size(); i++)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = g_GenGSConstBufferInfo[i]->size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.Usage = D3D11_USAGE_DEFAULT;

		hr = g_pD3DDevice->CreateBuffer(&bd, NULL, &g_GenGSConstantBufferBlob[i]);
		if (FAILED(hr))
			return hr;
	}

	for (int i = 0; i < g_DrawGSConstBufferInfo.size(); i++)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = g_DrawGSConstBufferInfo[i]->size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.Usage = D3D11_USAGE_DEFAULT;

		hr = g_pD3DDevice->CreateBuffer(&bd, NULL, &g_DrawGSConstantBufferBlob[i]);
		if (FAILED(hr))
			return hr;
	}
	
	CreateRandomTexture(g_pD3DDevice);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = g_pD3DDevice->CreateSamplerState(&sampDesc, &g_pSamPoint);
	if (FAILED(hr))
		return hr;


	
	if (FAILED(hr = D3DX11CreateShaderResourceViewFromFile( g_pD3DDevice, "..\\media\\particle.dds", NULL, 
		NULL, &g_txDiffuse, NULL )))
		return S_FALSE;
	//create the sample state
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pD3DDevice->CreateSamplerState(&sampDesc, &g_samLinear);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrix
	g_World = XMMatrixIdentity();

	// Initialize the view matrix

	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -50.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 50.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, g_Width / (FLOAT)g_Height, 0.01f, 100.0f );
	return S_OK;
}

bool g_bFirst = true;

bool AdvanceParticles( ID3D11Device* pd3dDevice, float fGlobalTime, 
	float fElapsedTime, XMFLOAT3 vGravity )
{
	// Set IA parameters
	ID3D11Buffer* pBuffers[1];
	if( g_bFirst )
		pBuffers[0] = g_pLauncherBuffer;
	else
		pBuffers[0] = g_pParticleDrawFrom;
	UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
	UINT offset[1] = { 0 };
	

	D3D11_DEPTH_STENCIL_DESC  depth_stencil_desc;
	ID3D11DepthStencilState* pDSS0 = NULL;
	ID3D11DepthStencilState* pDSS1 = NULL;
	UINT ref = 0;
	g_pImmediateContext->OMGetDepthStencilState(&pDSS0, &ref);
	if (pDSS0)
	{
		pDSS0->GetDesc(&depth_stencil_desc);
	}
	else
	{
		ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
	}

	depth_stencil_desc.DepthEnable = FALSE;
	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	if(FAILED(g_pD3DDevice->CreateDepthStencilState(&depth_stencil_desc,&pDSS1)))
		return S_FALSE;

	g_pImmediateContext->OMSetDepthStencilState(pDSS1,ref);

	if(pDSS0)
		pDSS0->Release();
	pDSS1->Release();
	
	g_pImmediateContext->IASetInputLayout(g_pGenInputLayout);
	g_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
	g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

	// Point to the correct output buffer
	pBuffers[0] = g_pParticleStreamTo;
	g_pImmediateContext->SOSetTargets( 1, pBuffers, offset );

	vGravity.x *= fElapsedTime;
	vGravity.y *= fElapsedTime;
	vGravity.z *= fElapsedTime;

	for (int i = 0; i < g_GenGSConstBufferInfo.size(); i++)
	{
		int size = g_GenGSConstBufferInfo[i]->numOfVariables;
		for (int j = 0; j < size; j++)
		{
			string ch = g_GenGSConstBufferInfo[i]->variables[j].name;

			float	fSecondsPerFirework = 0.4f;
			int		iNumEmber1s = 100;
			float	fMaxEmber2s = 15.0f;


			if (!ch.compare("g_fGlobalTime"))
			{
				//这里的size暂时使用sizeof(g_view);
				g_GenGSConstBufferInfo[i]->setFloat(g_GenGSConstBufferInfo[i]->variables[j].offset, fGlobalTime);
			}
			else if (!ch.compare("g_fElapsedTime"))
			{
				g_GenGSConstBufferInfo[i]->setFloat(g_GenGSConstBufferInfo[i]->variables[j].offset, fElapsedTime);
			}
			else if (!ch.compare("g_vFrameGravity"))
			{
				g_GenGSConstBufferInfo[i]->setVector(g_GenGSConstBufferInfo[i]->variables[j].offset,
					vGravity);
			}
			else if (!ch.compare("g_fSecondsPerFirework"))
			{
				g_GenGSConstBufferInfo[i]->setFloat(g_GenGSConstBufferInfo[i]->variables[j].offset,
					fSecondsPerFirework);
			}
			else if (!ch.compare("g_iNumEmber1s"))
			{
				g_GenGSConstBufferInfo[i]->setInt(g_GenGSConstBufferInfo[i]->variables[j].offset,
					iNumEmber1s);
			}
			else if (!ch.compare("g_fMaxEmber2s"))
			{
				g_GenGSConstBufferInfo[i]->setFloat(g_GenGSConstBufferInfo[i]->variables[j].offset,
					fMaxEmber2s);
			}

		}
		g_pImmediateContext->UpdateSubresource(g_GenGSConstantBufferBlob[i], 0, 0, 
			g_GenGSConstBufferInfo[i]->mSystemMemoryBuffer,0, 0);
	}

	g_pImmediateContext->VSSetShader(g_pParticlesGenVS, NULL, 0);
	g_pImmediateContext->GSSetShader(g_pParticlesGenGS, NULL, 0);
	g_pImmediateContext->GSSetShaderResources(g_TxRadomInfo.slot, 1, &g_pTxRadom);
	g_pImmediateContext->GSSetSamplers(g_SamPointInfo.slot, 1, &g_pSamPoint);
	for (int i = 0; i < g_GenGSConstBufferInfo.size(); i++)
	{
		g_pImmediateContext->GSSetConstantBuffers(g_GenGSConstBufferInfo[i]->slot,
			1, &g_GenGSConstantBufferBlob[i]);
	}
	
	g_pImmediateContext->PSSetShader(NULL, NULL, NULL);

	if( g_bFirst )
		g_pImmediateContext->Draw( 1, 0 );
	else
		g_pImmediateContext->DrawAuto();

	// Get back to normal
	pBuffers[0] = NULL;
	g_pImmediateContext->SOSetTargets( 1, pBuffers, offset );

	// Swap particle buffers
	ID3D11Buffer* pTemp = g_pParticleDrawFrom;
	g_pParticleDrawFrom = g_pParticleStreamTo;
	g_pParticleStreamTo = pTemp;

	g_bFirst = false;

	return true;
}

bool RenderParticles( ID3D11Device* pd3dDevice, XMMATRIX& mView, XMMATRIX& mProj )
{
	// Set IA parameters
	ID3D11Buffer* pBuffers[1] = { g_pParticleDrawFrom };

	UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
	UINT offset[1] = { 0 };



	D3D11_DEPTH_STENCIL_DESC  depth_stencil_desc;
	ID3D11DepthStencilState* pDSS0 = NULL;
	ID3D11DepthStencilState* pDSS1 = NULL;
	UINT ref = 0;
	g_pImmediateContext->OMGetDepthStencilState(&pDSS0, &ref);
	if (pDSS0)
	{
		pDSS0->GetDesc(&depth_stencil_desc);
	}
	else
	{
		ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
	}

	depth_stencil_desc.DepthEnable = FALSE;
	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	if(FAILED(g_pD3DDevice->CreateDepthStencilState(&depth_stencil_desc,&pDSS1)))
		return S_FALSE;

	g_pImmediateContext->OMSetDepthStencilState(pDSS1,ref);

	if(pDSS0)
		pDSS0->Release();
	pDSS1->Release();


	D3D11_BLEND_DESC blend_desc;
	ZeroMemory(&blend_desc, sizeof(D3D11_BLEND_DESC));
	blend_desc.AlphaToCoverageEnable = FALSE;
	D3D11_RENDER_TARGET_BLEND_DESC rt_blend_desc;
	rt_blend_desc.BlendEnable = TRUE;
	rt_blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rt_blend_desc.DestBlend = D3D11_BLEND_ONE;
	rt_blend_desc.BlendOp  = D3D11_BLEND_OP_ADD;
	rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ZERO;
	rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rt_blend_desc.RenderTargetWriteMask = 0x0F;
	blend_desc.RenderTarget[0] = rt_blend_desc;
	ID3D11BlendState* blend_state = NULL;
	if (FAILED(g_pD3DDevice->CreateBlendState(&blend_desc, &blend_state)))
		return false;
	float blendfactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pImmediateContext->OMSetBlendState(blend_state, blendfactor, 0xFFFFFFFF);

	blend_state->Release();


	g_pImmediateContext->IASetInputLayout(g_pDrawInputLayout);
	g_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
	g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

	// Set Effects Parameters
	XMMATRIX mWorldViewProj = XMMatrixMultiply(mView, mProj);
	XMVECTOR determinant;
	XMMATRIX mInvView = XMMatrixInverse(&determinant, mView);

	
	XMFLOAT3 g_positions[4] =
	{
		XMFLOAT3(-1, 1, 0),
		XMFLOAT3(1, 1, 0),
		XMFLOAT3(-1, -1, 0),
		XMFLOAT3(1, -1, 0),
	};
	XMFLOAT2 g_texcoords[4] = 
	{ 
		XMFLOAT2(0,1), 
		XMFLOAT2(1,1),
		XMFLOAT2(0,0),
		XMFLOAT2(1,0),
	};

	for (int i = 0; i < g_DrawGSConstBufferInfo.size(); i++)
	{
		int size = g_DrawGSConstBufferInfo[i]->numOfVariables;
		for (int j = 0; j < size; j++)
		{
			string ch = g_DrawGSConstBufferInfo[i]->variables[j].name;

			if (!ch.compare("g_positions"))
			{
				//这里的size暂时使用sizeof(g_view);
				g_DrawGSConstBufferInfo[i]->setVectorArray(g_DrawGSConstBufferInfo[i]->variables[j].offset,
					g_positions, 4);
			}
			else if (!ch.compare("g_texcoords"))
			{
				g_DrawGSConstBufferInfo[i]->setVectorArray(g_DrawGSConstBufferInfo[i]->variables[j].offset,
					g_texcoords, 4);
			}
			else if (!ch.compare("g_mWorldViewProj"))
			{
				g_DrawGSConstBufferInfo[i]->setMatrix(g_DrawGSConstBufferInfo[i]->variables[j].offset, 
					XMMatrixTranspose(mWorldViewProj));
			}
			else if (!ch.compare("g_mInvView"))
			{
				g_DrawGSConstBufferInfo[i]->setMatrix(g_DrawGSConstBufferInfo[i]->variables[j].offset,
					XMMatrixTranspose(mInvView));
			}

		}
		g_pImmediateContext->UpdateSubresource(g_DrawGSConstantBufferBlob[i], 0, 0, 
			g_DrawGSConstBufferInfo[i]->mSystemMemoryBuffer,0, 0);
	}

	
	g_pImmediateContext->VSSetShader(g_pParticlesDrawVS, NULL, 0);
	g_pImmediateContext->GSSetShader(g_pParticlesDrawGS, NULL, 0);
	
	g_pImmediateContext->GSSetConstantBuffers(g_DrawGSConstBufferInfo[0]->slot,
		1, &g_DrawGSConstantBufferBlob[0]);
	g_pImmediateContext->GSSetConstantBuffers(g_DrawGSConstBufferInfo[1]->slot,
		1, &g_DrawGSConstantBufferBlob[1]);
	
	g_pImmediateContext->PSSetShader(g_pParitclesDrawPS, NULL, NULL);
	g_pImmediateContext->PSSetShaderResources(g_txDiffuseInfo.slot, 1, &g_txDiffuse);
	g_pImmediateContext->PSSetSamplers(g_samLinearInfo.slot, 1, &g_samLinear);

	g_pImmediateContext->DrawAuto();
	return true;
}

//执行渲染工作;
void    Render(float fTime, float deltaTime)
{
	// Advance the Particles
	XMFLOAT3 vGravity(0.0f ,-9.8f, 0);
	AdvanceParticles( g_pD3DDevice, fTime, deltaTime, vGravity );
	//clear the back buffer
	float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	// Render the particles
	RenderParticles( g_pD3DDevice, g_View, g_Projection );
	
}



#define SAFE_RELEASE(x) if(x) x->Release()
void ClearScene()
{

	for(int i = 0; i < GEN_GS_CONSTBUFFER_SIZE; i++)
	{
		if(g_GenGSConstantBufferBlob[i])
			g_GenGSConstantBufferBlob[i]->Release();
	}
	for(int i = 0; i < DRAW_GS_CONSTBUFFER_SIZE; i++)
	{
		if(g_DrawGSConstantBufferBlob[i])
			g_DrawGSConstantBufferBlob[i]->Release();
	}
	SAFE_RELEASE(g_pParticlesGenVS);
	SAFE_RELEASE(g_pParticlesGenGS);
	SAFE_RELEASE(g_pLauncherBuffer);
	SAFE_RELEASE(g_pParticleDrawFrom);
	SAFE_RELEASE(g_pParticleStreamTo);
	SAFE_RELEASE(g_pGenInputLayout);
	SAFE_RELEASE(g_pTxRadom);
	SAFE_RELEASE(g_pSamPoint);
	SAFE_RELEASE(g_pParticlesDrawVS);
	SAFE_RELEASE(g_pParticlesDrawGS);
	SAFE_RELEASE(g_pParitclesDrawPS);
	SAFE_RELEASE(g_pDrawInputLayout);
	SAFE_RELEASE(g_txDiffuse);
	SAFE_RELEASE(g_samLinear);
}