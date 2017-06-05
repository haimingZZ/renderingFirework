//--------------------------------------------------------------------------------------
// File: Tutorial00.cpp
//
// This tutorial sets up a windows application with a window and a message pump
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <xnamath.h>
#include "resource.h"
#include <fstream>
using namespace std;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE   g_hInst		= NULL;
HWND        g_hWnd		= NULL;
int			g_Width		= 0;
int         g_Height	= 0;

ID3D11Device*			g_pD3DDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL;
IDXGISwapChain*			g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

ID3D11VertexShader*		g_pVertexShader = NULL;
ID3D11GeometryShader*	g_pGeometryShader=NULL;
ID3D11PixelShader*		g_pPixelShader  = NULL;
ID3D11Buffer*			g_pVertexBuffer = NULL;
ID3D11Buffer*			g_pIndexBuffer	= NULL;
ID3D11InputLayout*		g_pInputLayout  = NULL;

ID3D11Texture2D*		g_pDepthStencil	= NULL;
ID3D11DepthStencilView*	g_pDepthStencilView = NULL;

XMMATRIX				g_World;
XMMATRIX				g_View;
XMMATRIX				g_Projection;

ID3D11ShaderResourceView*	g_pTextureRV = NULL;
ID3D11SamplerState*			g_pSamplerLinear = NULL;
XMFLOAT4				g_vMeshColor;


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
void    Render();

//初始化场景;
HRESULT InitScene();
//清除场景;
void	ClearScene();


struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

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
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			Render();
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
	wcex.lpszClassName = "RenderATriangleWindowClass";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_DIRECTX11 );
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	g_Width = width;
	g_Height= height;
	RECT rc = { 0, 0, width, height};
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	g_hWnd = CreateWindow( "RenderATriangleWindowClass", "Render a triangle", WS_OVERLAPPEDWINDOW,
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

#include <vector>
#include <string>
using namespace std;

struct VariableInfo
{
	int		size;
	int		offset;
	int     slot;
	string	name;
};

struct ConstantBufferInfo
{
	int slot;
	int size;
	int numOfVariables;
	string name;
	vector<VariableInfo> variables;
};

#define VS_CONSTBUFFER_SIZE 1
#define GS_CONSTBUFFER_SIZE 1
#define PS_CONSTBUFFER_SIZE 1

ConstantBufferInfo g_VSConstantBuffers[VS_CONSTBUFFER_SIZE];
ConstantBufferInfo g_PSConstantBuffers2[PS_CONSTBUFFER_SIZE];

ConstantBufferInfo	g_GSConstantBuffers[GS_CONSTBUFFER_SIZE];
ID3D11Buffer*		g_GSConstantBufferBlob[GS_CONSTBUFFER_SIZE];

ID3D11Buffer*		 g_VSConstantBufferBlob[VS_CONSTBUFFER_SIZE];
ID3D11Buffer*		 g_PSConstantBuffer2Blob[PS_CONSTBUFFER_SIZE];

VariableInfo g_PSTextureView;
VariableInfo g_PSSampler;

//初始化场景;
HRESULT InitScene()
{
	HRESULT hr;
	//compile the vertex buffer
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile("TriangleShader.fx", "VS", "vs_4_0", &pVSBlob);
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	hr = g_pD3DDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}
	 
	ID3D11ShaderReflection* pReflector = NULL;

	hr = D3DReflect(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), IID_ID3D11ShaderReflection,
		(void**)&pReflector);

	if(FAILED(hr))
	{
		pVSBlob->Release();
		if(pReflector != NULL)
			pReflector->Release();
		return hr;
	}

	D3D11_SHADER_DESC desc;
	pReflector->GetDesc(&desc);

	int bufferC = 0;
	for (UINT k = 0; k < desc.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pReflector->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_CBUFFER)
		{
			ConstantBufferInfo bufferInfo;
			bufferInfo.slot = input_bind_desc.BindPoint;

			D3D11_SHADER_BUFFER_DESC shader_buffer_desc;
			ID3D11ShaderReflectionConstantBuffer* pConstBuffer = 
				pReflector->GetConstantBufferByName(input_bind_desc.Name);
			pConstBuffer->GetDesc(&shader_buffer_desc);

			bufferInfo.name = shader_buffer_desc.Name;
			bufferInfo.numOfVariables = shader_buffer_desc.Variables;
			bufferInfo.size = shader_buffer_desc.Size;

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

				bufferInfo.variables.push_back(variableInfo);
			}
			g_VSConstantBuffers[bufferC++] = bufferInfo;
		}
	}

	pReflector->Release();
	
	
	//define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
		}
	};
	UINT numElements = ARRAYSIZE(layout);

	//create the input layout
	hr = g_pD3DDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pInputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;
	//set the input layout
	g_pImmediateContext->IASetInputLayout(g_pInputLayout);

	/************************************************************************/
	// Geometry Shader   
	/************************************************************************/

	ID3DBlob* pGSBlob = NULL;
	hr = CompileShaderFromFile("TriangleShader.fx", "GS", "gs_4_0", &pGSBlob);
	if (FAILED(hr))
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	//create the geometry shader
	hr = g_pD3DDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &g_pGeometryShader);


	ID3D11ShaderReflection* pGSReflector = NULL;

	hr = D3DReflect(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), IID_ID3D11ShaderReflection,
		(void**)&pGSReflector);

	if(FAILED(hr))
	{
		pGSBlob->Release();
		if(pGSReflector != NULL)
			pGSReflector->Release();
		return hr;
	}

	D3D11_SHADER_DESC gs_desc;
	pGSReflector->GetDesc(&gs_desc);

	int gs_desc_bufferC = 0;
	for (UINT k = 0; k < gs_desc.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pGSReflector->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_CBUFFER)
		{
			ConstantBufferInfo bufferInfo;
			bufferInfo.slot = input_bind_desc.BindPoint;

			D3D11_SHADER_BUFFER_DESC shader_buffer_desc;
			ID3D11ShaderReflectionConstantBuffer* pConstBuffer = 
				pGSReflector->GetConstantBufferByName(input_bind_desc.Name);
			pConstBuffer->GetDesc(&shader_buffer_desc);

			bufferInfo.name = shader_buffer_desc.Name;
			bufferInfo.numOfVariables = shader_buffer_desc.Variables;
			bufferInfo.size = shader_buffer_desc.Size;

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

				bufferInfo.variables.push_back(variableInfo);
			}
			g_GSConstantBuffers[gs_desc_bufferC++] = bufferInfo;
		}
	}

	pGSReflector->Release();
	pGSBlob->Release();
	/////////////////////////////////////////////////////////////////////////////////


	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile( "TriangleShader.fx", "PS", "ps_4_0", &pPSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}
	// Create the pixel shader
	hr = g_pD3DDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
	
	

	///////////////////////////////////////////////////////////////
	ID3D11ShaderReflection* pReflector1 = NULL;

	hr = D3DReflect(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), IID_ID3D11ShaderReflection,
		(void**)&pReflector1);

	if(FAILED(hr))
	{
		pPSBlob->Release();
		if(pReflector1 != NULL)
			pReflector1->Release();
		return hr;
	}

	D3D11_SHADER_DESC desc1;
	pReflector1->GetDesc(&desc1);

	int buffercount = 0;
	for (UINT k = 0; k < desc1.BoundResources; k++)
	{
		D3D11_SHADER_INPUT_BIND_DESC input_bind_desc;
		pReflector1->GetResourceBindingDesc(k, &input_bind_desc);
		if (input_bind_desc.Type == D3D_SIT_CBUFFER)
		{
			ConstantBufferInfo bufferInfo;
			bufferInfo.slot = input_bind_desc.BindPoint;

			ID3D11ShaderReflectionConstantBuffer* pConstBuffer = 
				pReflector1->GetConstantBufferByName(input_bind_desc.Name);

			D3D11_SHADER_BUFFER_DESC shader_buffer_desc;
			pConstBuffer->GetDesc(&shader_buffer_desc);

			bufferInfo.name = shader_buffer_desc.Name;
			bufferInfo.numOfVariables = shader_buffer_desc.Variables;
			bufferInfo.size = shader_buffer_desc.Size;

			//load the description of each variable for use later on when binding a buffer
			for (UINT j = 0; j < shader_buffer_desc.Variables; j++)
			{
				VariableInfo variableInfo;
				//get the variable description and store it
				ID3D11ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC var_desc;
				pVariable->GetDesc(&var_desc);

				variableInfo.name = var_desc.Name;
				variableInfo.offset = var_desc.StartOffset;
				variableInfo.size = var_desc.Size;

				bufferInfo.variables.push_back(variableInfo);
			}
			g_PSConstantBuffers2[buffercount++] = bufferInfo;
		}
		else if (input_bind_desc.Type == D3D_SIT_TEXTURE)
		{
			g_PSTextureView.slot = input_bind_desc.BindPoint;	
			g_PSTextureView.name = input_bind_desc.Name;
		}
		else if (input_bind_desc.Type == D3D_SIT_SAMPLER)
		{
			g_PSSampler.slot = input_bind_desc.BindPoint;
			g_PSSampler.name = input_bind_desc.Name;
		}
	}

	pReflector1->Release();
	//////////////////////////////////////////////////////////////
	
	pPSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	//顶点和索引数据;
// 	SimpleVertex vertices[] = 
// 	{
// 		//左面;
// 		XMFLOAT3(-1.0, -1.0, -1.0), XMFLOAT2(0.0f, 1.0f),
// 		XMFLOAT3(-1.0, -1.0, 1.0),  XMFLOAT2(1.0f, 1.0f),
// 		XMFLOAT3(-1.0, 1.0, 1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(-1.0, 1.0, -1.0),	XMFLOAT2(0.0f, 0.0f),
// 
// 		//右面;
// 		XMFLOAT3(1.0, -1.0, 1.0),	XMFLOAT2(0.0f, 1.0f),
// 		XMFLOAT3(1.0, -1.0, -1.0),	XMFLOAT2(1.0f, 1.0f),
// 		XMFLOAT3(1.0, 1.0, -1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(1.0, 1.0, 1.0),	XMFLOAT2(0.0f, 0.0f),
// 
// 		//上面;
// 		XMFLOAT3(-1.0, 1.0, 1.0),	XMFLOAT2(0.0f, 1.0f),
// 		XMFLOAT3(1.0, 1.0, 1.0),	XMFLOAT2(1.0f, 1.0f),
// 		XMFLOAT3(1.0, 1.0, -1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(-1.0, 1.0, -1.0),	XMFLOAT2(0.0f, 0.0f),
// 
// 		//下面;
// 		XMFLOAT3(-1.0, -1.0, -1.0),	XMFLOAT2(0.0f, 0.0f),
// 		XMFLOAT3(1.0, -1.0, -1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(1.0, -1.0, 1.0),	XMFLOAT2(1.0f, 1.0f),
// 		XMFLOAT3(-1.0, -1.0, 1.0),	XMFLOAT2(0.0f, 1.0f),
// 
// 		//前面;
// 		XMFLOAT3(-1.0, -1.0, 1.0),	XMFLOAT2(0.0f, 0.0f),
// 		XMFLOAT3(1.0, -1.0, 1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(1.0, 1.0, 1.0),	XMFLOAT2(1.0f, 1.0f),
// 		XMFLOAT3(-1.0, 1.0, 1.0),	XMFLOAT2(0.0f, 1.0f),
// 
// 		//后面;
// 		XMFLOAT3(-1.0, -1.0, -1.0),	XMFLOAT2(0.0f, 1.0f),
// 		XMFLOAT3(-1.0, 1.0, -1.0),	XMFLOAT2(0.0f, 0.0f),
// 		XMFLOAT3(1.0, 1.0, -1.0),	XMFLOAT2(1.0f, 0.0f),
// 		XMFLOAT3(1.0, -1.0, -1.0),	XMFLOAT2(1.0f, 1.0f),
// 	};
// 
// 	WORD indices[36] =
// 	{
// 		//左面;
// 		0, 1, 2,
// 		0, 2, 3,
// 		//右面;
// 		4, 5, 6,
// 		4, 6, 7,
// 		//上面;
// 		8, 9, 10,
// 		8, 10, 11,
// 		//下面;
// 		12, 13, 14,
// 		12, 14, 15,
// 		//前面;
// 		16, 17, 18,
// 		16, 18, 19,
// 		//后面;
// 		20, 21, 22,
// 		20, 22, 23
// 	};



	//Create vertex buffer
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	if(FAILED(g_pD3DDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer)))
		return FALSE;

	//set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//create index buffer
	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = indices;
	if (FAILED(g_pD3DDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer)))
		return FALSE;
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	//set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (int i = 0; i < VS_CONSTBUFFER_SIZE; i++)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = g_VSConstantBuffers[i].size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.Usage = D3D11_USAGE_DYNAMIC;

		hr = g_pD3DDevice->CreateBuffer(&bd, NULL, &g_VSConstantBufferBlob[i]);
		if (FAILED(hr))
			return hr;
	}

	for (int i = 0; i < GS_CONSTBUFFER_SIZE; i++)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = g_GSConstantBuffers[i].size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.Usage = D3D11_USAGE_DYNAMIC;

		hr = g_pD3DDevice->CreateBuffer(&bd, NULL, &g_GSConstantBufferBlob[i]);
		if (FAILED(hr))
			return hr;
	}

	for (int i = 0; i < PS_CONSTBUFFER_SIZE; i++)
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = g_PSConstantBuffers2[i].size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.Usage = D3D11_USAGE_DYNAMIC;

		hr = g_pD3DDevice->CreateBuffer(&bd, NULL, &g_PSConstantBuffer2Blob[i]);
		if (FAILED(hr))
			return hr;
	}
	//create shader-resource view
	hr = D3DX11CreateShaderResourceViewFromFile(g_pD3DDevice, "seafloor.dds", NULL, NULL, &g_pTextureRV, NULL);
	if (FAILED(hr))
		return hr;
	//create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pD3DDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrix
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, 6.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, g_Width / (FLOAT)g_Height, 0.01f, 100.0f );
}

//执行渲染工作;
void    Render()
{
	//update the time
	static float t = 0.0f;
	static DWORD lasttime = GetTickCount();
	DWORD curtime = GetTickCount();
	t = (float)(curtime - lasttime) / 1000.0f;

	g_World = (XMMatrixRotationY(t));

	//clear the back buffer
	float clearColor[4] = {0.0f, 0.125f, 0.6f, 1.0f};
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Modify the color
	g_vMeshColor.x = ( sinf( t * 1.0f ) + 1.0f ) * 0.5f;
	g_vMeshColor.y = ( cosf( t * 3.0f ) + 1.0f ) * 0.5f;
	g_vMeshColor.z = ( sinf( t * 5.0f ) + 1.0f ) * 0.5f;

	for (int i = 0; i < VS_CONSTBUFFER_SIZE; i++)
	{
		//char* buffer = new char[g_VSConstantBuffers[i].size];
		D3D11_MAPPED_SUBRESOURCE mappedSource;
		HRESULT hr = g_pImmediateContext->Map(g_VSConstantBufferBlob[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSource);
		//ZeroMemory(buffer, g_VSConstantBuffers[i].size);
		int size = g_VSConstantBuffers[i].numOfVariables;
		for (int j = 0; j < size; j++)
		{
			string ch = g_VSConstantBuffers[i].variables[j].name;

			if (!ch.compare("View"))
			{
				//这里的size暂时使用sizeof(g_view);
				CopyMemory((char*)mappedSource.pData + g_VSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_View), sizeof(g_View));
			}
			else if (!ch.compare("Projection"))
			{
				CopyMemory((char*)mappedSource.pData + g_VSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_Projection), sizeof(g_Projection));
			}
			else if (!ch.compare("World"))
			{
				CopyMemory((char*)mappedSource.pData + g_VSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_World), sizeof(g_World));
			}
			else if (!ch.compare("vMeshColor"))
			{
				CopyMemory((char*)mappedSource.pData + g_VSConstantBuffers[i].variables[j].offset, 
					&g_vMeshColor, sizeof(g_vMeshColor));
			}
			
		}
		g_pImmediateContext->Unmap(g_VSConstantBufferBlob[i], 0);
	}


	for (int i = 0; i < GS_CONSTBUFFER_SIZE; i++)
	{
		//char* buffer = new char[g_VSConstantBuffers[i].size];
		D3D11_MAPPED_SUBRESOURCE mappedSource;
		HRESULT hr = g_pImmediateContext->Map(g_GSConstantBufferBlob[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSource);
		//ZeroMemory(buffer, g_VSConstantBuffers[i].size);
		int size = g_GSConstantBuffers[i].numOfVariables;
		for (int j = 0; j < size; j++)
		{
			string ch = g_GSConstantBuffers[i].variables[j].name;

			if (!ch.compare("View"))
			{
				//这里的size暂时使用sizeof(g_view);
				CopyMemory((char*)mappedSource.pData + g_GSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_View), sizeof(g_View));
			}
			else if (!ch.compare("Projection"))
			{
				CopyMemory((char*)mappedSource.pData + g_GSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_Projection), sizeof(g_Projection));
			}
			else if (!ch.compare("World"))
			{
				CopyMemory((char*)mappedSource.pData + g_GSConstantBuffers[i].variables[j].offset, 
					&XMMatrixTranspose(g_World), sizeof(g_World));
			}
			else if (!ch.compare("vMeshColor"))
			{
				CopyMemory((char*)mappedSource.pData + g_GSConstantBuffers[i].variables[j].offset, 
					&g_vMeshColor, sizeof(g_vMeshColor));
			}

		}
		g_pImmediateContext->Unmap(g_GSConstantBufferBlob[i], 0);
	}

	for (int i = 0; i < PS_CONSTBUFFER_SIZE; i++)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSource;
		g_pImmediateContext->Map(g_PSConstantBuffer2Blob[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSource);
		int size = g_PSConstantBuffers2[i].numOfVariables;
		for (int j = 0; j < size; j++)
		{
			string ch = g_PSConstantBuffers2[i].variables[j].name;
			if (!ch.compare("View"))
			{
				//这里的size暂时使用sizeof(g_view);
				CopyMemory((char*)mappedSource.pData + g_PSConstantBuffers2[i].variables[j].offset, 
					&XMMatrixTranspose(g_View), sizeof(g_View));
			}
			else if (!ch.compare("Projection"))
			{
				CopyMemory((char*)mappedSource.pData + g_PSConstantBuffers2[i].variables[j].offset, 
					&XMMatrixTranspose(g_Projection), sizeof(g_Projection));
			}
			else if (!ch.compare("World"))
			{
				CopyMemory((char*)mappedSource.pData + g_PSConstantBuffers2[i].variables[j].offset, 
					&XMMatrixTranspose(g_World), sizeof(g_World));
			}
			else if (!ch.compare("vMeshColor"))
			{
				CopyMemory((char*)mappedSource.pData + g_PSConstantBuffers2[i].variables[j].offset, 
					&g_vMeshColor, sizeof(g_vMeshColor));
			}
		}
		g_pImmediateContext->Unmap(g_PSConstantBuffer2Blob[i], 0);
	}

	//update variables that change once per frame
	//render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	for(int i = 0; i < VS_CONSTBUFFER_SIZE; i++)
	{
		g_pImmediateContext->VSSetConstantBuffers(g_VSConstantBuffers[i].slot, 1, &g_VSConstantBufferBlob[i]);
	}

	g_pImmediateContext->GSSetShader(g_pGeometryShader, NULL, 0);
	for (int i = 0; i < GS_CONSTBUFFER_SIZE; i++)
	{
		g_pImmediateContext->GSSetConstantBuffers(g_GSConstantBuffers[i].slot, 1, &g_GSConstantBufferBlob[i]);
	}

	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	for (int i = 0; i < PS_CONSTBUFFER_SIZE; i++)
	{
		g_pImmediateContext->PSSetConstantBuffers(g_PSConstantBuffers2[i].slot, 1, &g_PSConstantBuffer2Blob[i]);
	}
	g_pImmediateContext->PSSetShaderResources(g_PSTextureView.slot, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetSamplers(g_PSSampler.slot, 1, &g_pSamplerLinear);
	g_pImmediateContext->DrawIndexed(36, 0, 0);


	g_pSwapChain->Present(0, 0);
}

void ClearScene()
{

// 	for(int i = 0; i < ; i++)
// 	{
// 		if(g_VSConstantBufferBlob[i])
// 			g_VSConstantBufferBlob[i]->Release();
// 	}
// 	for(int i = 0; i < 2; i++)
// 	{
// 		if(g_PSConstantBuffer2Blob[i])
// 			g_PSConstantBuffer2Blob[i]->Release();
// 	}

	if( g_pVertexBuffer ) g_pVertexBuffer->Release();
	if( g_pIndexBuffer )  g_pIndexBuffer->Release();
	if( g_pInputLayout ) g_pInputLayout->Release();
	if( g_pVertexShader ) g_pVertexShader->Release();
	if( g_pPixelShader ) g_pPixelShader->Release();
	if( g_pTextureRV ) g_pTextureRV->Release();
	if( g_pSamplerLinear ) g_pSamplerLinear->Release();
}