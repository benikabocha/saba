#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wrl.h>
#include <d3d11.h>

#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Time.h>
#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VMDCameraAnimation.h>

#include "./resource_dx/mmd.vso.h"
#include "./resource_dx/mmd.pso.h"
#include "./resource_dx/mmd_edge.vso.h"
#include "./resource_dx/mmd_edge.pso.h"
#include "./resource_dx/mmd_ground_shadow.vso.h"
#include "./resource_dx/mmd_ground_shadow.pso.h"

void Usage()
{
	std::cout << "app [-model <pmd|pmx file path>] [-vmd <vmd file path>]\n";
	std::cout << "e.g. app -model model1.pmx -vmd anim1_1.vmd -vmd anim1_2.vmd  -model model2.pmx\n";
}

// vertex

struct Vertex
{
	glm::vec3	m_position;
	glm::vec3	m_normal;
	glm::vec2	m_uv;
};

// mmd shader constant buffer

struct MMDVertexShaderCB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
};

struct MMDPixelShaderCB
{
	float		m_alpha;
	glm::vec3	m_diffuse;
	glm::vec3	m_ambient;
	float		m_dummy1;
	glm::vec3	m_specular;
	float		m_specularPower;
	glm::vec3	m_lightColor;
	float		m_dummy2;
	glm::vec3	m_lightDir;
	float		m_dummy3;

	glm::vec4	m_texMulFactor;
	glm::vec4	m_texAddFactor;

	glm::vec4	m_toonTexMulFactor;
	glm::vec4	m_toonTexAddFactor;

	glm::vec4	m_sphereTexMulFactor;
	glm::vec4	m_sphereTexAddFactor;

	glm::ivec4	m_textureModes;

};

// mmd edge shader constant buffer

struct MMDEdgeVertexShaderCB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
	glm::vec2	m_screenSize;
	float		m_dummy[2];
};

struct MMDEdgeSizeVertexShaderCB
{
	float		m_edgeSize;
	float		m_dummy[3];
};

struct MMDEdgePixelShaderCB
{
	glm::vec4	m_edgeColor;
};


// mmd ground shadow shader constant buffer

struct MMDGroundShadowVertexShaderCB
{
	glm::mat4	m_wvp;
};

struct MMDGroundShadowPixelShaderCB
{
	glm::vec4	m_shadowColor;
};

struct Texture
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Texture2D>				m_texture;
	ComPtr<ID3D11ShaderResourceView>	m_textureView;
	bool								m_hasAlpha;
};

struct AppContext
{
	std::string m_resourceDir;
	std::string	m_shaderDir;
	std::string	m_mmdDir;

	glm::mat4	m_viewMat;
	glm::mat4	m_projMat;
	int			m_screenWidth = 0;
	int			m_screenHeight = 0;

	UINT		m_multiSampleCount = 1;
	UINT		m_multiSampleQuality = 0;

	glm::vec3	m_lightColor = glm::vec3(1, 1, 1);
	glm::vec3	m_lightDir = glm::vec3(-0.5f, -1.0f, -0.5f);

	float	m_elapsed = 0.0f;
	float	m_animTime = 0.0f;
	std::unique_ptr<saba::VMDCameraAnimation>	m_vmdCameraAnim;

	std::map<std::string, Texture>	m_textures;

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Device>			m_device;
	ComPtr<ID3D11RenderTargetView>	m_renderTargetView;
	ComPtr <ID3D11DepthStencilView> m_depthStencilView;

	ComPtr<ID3D11VertexShader>	m_mmdVS;
	ComPtr<ID3D11PixelShader>	m_mmdPS;
	ComPtr<ID3D11InputLayout>	m_mmdInputLayout;
	ComPtr<ID3D11SamplerState>	m_textureSampler;
	ComPtr<ID3D11SamplerState>	m_toonTextureSampler;
	ComPtr<ID3D11SamplerState>	m_sphereTextureSampler;
	ComPtr<ID3D11BlendState>	m_mmdBlendState;
	ComPtr<ID3D11RasterizerState>	m_mmdFrontFaceRS;
	ComPtr<ID3D11RasterizerState>	m_mmdBothFaceRS;

	ComPtr<ID3D11VertexShader>	m_mmdEdgeVS;
	ComPtr<ID3D11PixelShader>	m_mmdEdgePS;
	ComPtr<ID3D11InputLayout>	m_mmdEdgeInputLayout;
	ComPtr<ID3D11BlendState>	m_mmdEdgeBlendState;
	ComPtr<ID3D11RasterizerState>	m_mmdEdgeRS;

	ComPtr<ID3D11VertexShader>	m_mmdGroundShadowVS;
	ComPtr<ID3D11PixelShader>	m_mmdGroundShadowPS;
	ComPtr<ID3D11InputLayout>	m_mmdGroundShadowInputLayout;
	ComPtr<ID3D11BlendState>	m_mmdGroundShadowBlendState;
	ComPtr<ID3D11RasterizerState>	m_mmdGroundShadowRS;
	ComPtr<ID3D11DepthStencilState>	m_mmdGroundShadowDSS;

	ComPtr<ID3D11DepthStencilState>	m_defaultDSS;

	ComPtr<ID3D11Texture2D>				m_dummyTexture;
	ComPtr<ID3D11ShaderResourceView>	m_dummyTextureView;
	ComPtr<ID3D11SamplerState>			m_dummySampler;

	bool Setup(ComPtr<ID3D11Device> device);
	bool CreateShaders();

	Texture GetTexture(const std::string& texturePath);
};

bool AppContext::Setup(ComPtr<ID3D11Device> device)
{
	m_device = device;

	m_resourceDir = saba::PathUtil::GetExecutablePath();
	m_resourceDir = saba::PathUtil::GetDirectoryName(m_resourceDir);
	m_resourceDir = saba::PathUtil::Combine(m_resourceDir, "resource");
	m_shaderDir = saba::PathUtil::Combine(m_resourceDir, "shader");
	m_mmdDir = saba::PathUtil::Combine(m_resourceDir, "mmd");

	if (!CreateShaders())
	{
		return false;
	}

	return true;
}

bool AppContext::CreateShaders()
{
	HRESULT hr;

	// mmd shader
	hr = m_device->CreateVertexShader(mmd_vso_data, sizeof(mmd_vso_data), nullptr, &m_mmdVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_device->CreatePixelShader(mmd_pso_data, sizeof(mmd_pso_data), nullptr, &m_mmdPS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = m_device->CreateInputLayout(
		mmdInputElementDesc, 3,
		mmd_vso_data, sizeof(mmd_vso_data),
		&m_mmdInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Texture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = m_device->CreateSamplerState(&samplerDesc, &m_textureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// ToonTexture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = m_device->CreateSamplerState(&samplerDesc, &m_toonTextureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// SphereTexture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = m_device->CreateSamplerState(&samplerDesc, &m_sphereTextureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = m_device->CreateBlendState(&blendDesc, &m_mmdBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State (Front face)
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_BACK;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdFrontFaceRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State (Both face)
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdBothFaceRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// mmd edge shader
	hr = m_device->CreateVertexShader(mmd_edge_vso_data, sizeof(mmd_edge_vso_data), nullptr, &m_mmdEdgeVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_device->CreatePixelShader(mmd_edge_pso_data, sizeof(mmd_edge_pso_data), nullptr, &m_mmdEdgePS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdEdgeInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = m_device->CreateInputLayout(
		mmdEdgeInputElementDesc, 2,
		mmd_edge_vso_data, sizeof(mmd_edge_vso_data),
		&m_mmdEdgeInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = m_device->CreateBlendState(&blendDesc, &m_mmdEdgeBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_FRONT;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdEdgeRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// mmd ground shadow shader
	hr = m_device->CreateVertexShader(mmd_ground_shadow_vso_data, sizeof(mmd_ground_shadow_vso_data), nullptr, &m_mmdGroundShadowVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_device->CreatePixelShader(mmd_ground_shadow_pso_data, sizeof(mmd_ground_shadow_pso_data), nullptr, &m_mmdGroundShadowPS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdGroundShadowInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = m_device->CreateInputLayout(
		mmdGroundShadowInputElementDesc, 1,
		mmd_ground_shadow_vso_data, sizeof(mmd_ground_shadow_vso_data),
		&m_mmdGroundShadowInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = m_device->CreateBlendState(&blendDesc, &m_mmdGroundShadowBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = -1;
		rsDesc.SlopeScaledDepthBias = -1.0f;
		rsDesc.DepthBiasClamp = -1.0f;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdGroundShadowRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = 0x01;
		dsDesc.StencilWriteMask = 0xFF;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		hr = m_device->CreateDepthStencilState(&dsDesc, &m_mmdGroundShadowDSS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Default Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = false;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		hr = m_device->CreateDepthStencilState(&dsDesc, &m_defaultDSS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Dummy texture
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = 1;
		tex2dDesc.Height = 1;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		hr = m_device->CreateTexture2D(&tex2dDesc, nullptr, &m_dummyTexture);
		if (FAILED(hr))
		{
			return false;
		}

		hr = m_device->CreateShaderResourceView(m_dummyTexture.Get(), nullptr, &m_dummyTextureView);
		if (FAILED(hr))
		{
			return false;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = m_device->CreateSamplerState(&samplerDesc, &m_dummySampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

Texture AppContext::GetTexture(const std::string& texturePath)
{
	auto it = m_textures.find(texturePath);
	if (it == m_textures.end())
	{
		saba::File file;
		if (!file.Open(texturePath))
		{
			return Texture();
		}
		int x, y, comp;
		int ret = stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp);
		if (ret == 0)
		{
			return Texture();
		}

		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = x;
		tex2dDesc.Height = y;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;

		int reqComp = 0;
		bool hasAlpha = false;
		if (comp != 4)
		{
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			hasAlpha = false;
		}
		else
		{
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			hasAlpha = true;
		}
		uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, STBI_rgb_alpha);
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = image;
		initData.SysMemPitch = 4 * x;
		ComPtr<ID3D11Texture2D> tex2d;
		HRESULT hr = m_device->CreateTexture2D(&tex2dDesc, &initData, &tex2d);
		stbi_image_free(image);
		if (FAILED(hr))
		{
			return Texture();
		}

		ComPtr<ID3D11ShaderResourceView> tex2dRV;
		hr = m_device->CreateShaderResourceView(tex2d.Get(), nullptr, &tex2dRV);
		if (FAILED(hr))
		{
			return Texture();
		}

		Texture tex;
		tex.m_texture = tex2d;
		tex.m_textureView = tex2dRV;
		tex.m_hasAlpha = hasAlpha;

		m_textures[texturePath] = tex;

		return m_textures[texturePath];
	}
	else
	{
		return (*it).second;
	}
}

struct Input
{
	std::string					m_modelPath;
	std::vector<std::string>	m_vmdPaths;
};


struct Material
{
	explicit Material(const saba::MMDMaterial& mat)
		: m_mmdMat(mat)
	{}

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const saba::MMDMaterial&	m_mmdMat;
	Texture	m_texture;
	Texture	m_spTexture;
	Texture	m_toonTexture;
};

struct Model
{

	std::shared_ptr<saba::MMDModel>	m_mmdModel;
	std::unique_ptr<saba::VMDAnimation>	m_vmdAnim;

	std::vector<Material>	m_materials;

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11DeviceContext>	m_context;
	ComPtr<ID3D11Buffer>		m_vertexBuffer;
	ComPtr<ID3D11Buffer>		m_indexBuffer;
	DXGI_FORMAT					m_indexBufferFormat;
	ComPtr<ID3D11Buffer>		m_mmdVSConstantBuffer;
	ComPtr<ID3D11Buffer>		m_mmdPSConstantBuffer;

	ComPtr<ID3D11Buffer>		m_mmdEdgeVSConstantBuffer;
	ComPtr<ID3D11Buffer>		m_mmdEdgeSizeVSConstantBuffer;
	ComPtr<ID3D11Buffer>		m_mmdEdgePSConstantBuffer;

	ComPtr<ID3D11Buffer>		m_mmdGroundShadowVSConstantBuffer;
	ComPtr<ID3D11Buffer>		m_mmdGroundShadowPSConstantBuffer;

	bool Setup(AppContext& appContext);

	void UpdateAnimation(const AppContext& appContext);
	void Update(const AppContext& appContext);
	void Draw(const AppContext& appContext);
};

/*
	Model
*/
bool Model::Setup(AppContext& appContext)
{
	auto t = sizeof(MMDPixelShaderCB);
	HRESULT hr;
	hr = appContext.m_device->CreateDeferredContext(0, &m_context);
	if (FAILED(hr))
	{
		return false;
	}

	// Setup vertex buffer
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.ByteWidth = UINT(sizeof(Vertex) * m_mmdModel->GetVertexCount());
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_vertexBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup index buffer;
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufDesc.ByteWidth = UINT(m_mmdModel->GetIndexElementSize() * m_mmdModel->GetIndexCount());
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = m_mmdModel->GetIndices();
		hr = appContext.m_device->CreateBuffer(&bufDesc, &initData, &m_indexBuffer);
		if (FAILED(hr))
		{
			return false;
		}

		if (1 == m_mmdModel->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R8_UINT;
		}
		else if (2 == m_mmdModel->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R16_UINT;
		}
		else if (4 == m_mmdModel->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R32_UINT;
		}
		else
		{
			return false;
		}
	}

	// Setup mmd vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDPixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdPSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgeVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgeVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge vertex shader constant buffer (VSEdgeData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgeSizeVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgeSizeVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgePixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgePSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd ground shadow vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDGroundShadowVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdGroundShadowVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd ground shadow pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDGroundShadowPixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = appContext.m_device->CreateBuffer(&bufDesc, nullptr, &m_mmdGroundShadowPSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup materials
	for (size_t i = 0; i < m_mmdModel->GetMaterialCount(); i++)
	{
		const auto& mmdMat = m_mmdModel->GetMaterials()[i];
		Material mat(mmdMat);
		if (!mmdMat.m_texture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_texture);
			mat.m_texture = tex;
		}
		if (!mmdMat.m_spTexture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_spTexture);
			mat.m_spTexture = tex;
		}
		if (!mmdMat.m_toonTexture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_toonTexture);
			mat.m_toonTexture = tex;
		}
		m_materials.emplace_back(std::move(mat));
	}

	return true;
}

void Model::UpdateAnimation(const AppContext& appContext)
{
	m_mmdModel->BeginAnimation();
	m_mmdModel->UpdateAllAnimation(m_vmdAnim.get(), appContext.m_animTime * 30.0f, appContext.m_elapsed);
	m_mmdModel->EndAnimation();
}

void Model::Update(const AppContext& appContext)
{
	m_mmdModel->Update();

	size_t vtxCount = m_mmdModel->GetVertexCount();
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE mapRes;
	hr = m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRes);
	if (FAILED(hr))
	{
		return;
	}
	Vertex* vertices = (Vertex*)mapRes.pData;
	const glm::vec3* positions = m_mmdModel->GetUpdatePositions();
	const glm::vec3* normals = m_mmdModel->GetUpdateNormals();
	const glm::vec2* uvs = m_mmdModel->GetUpdateUVs();
	for (size_t i = 0; i < vtxCount; i++)
	{
		vertices[i].m_position = positions[i];
		vertices[i].m_normal = normals[i];
		vertices[i].m_uv = uvs[i];
	}
	m_context->Unmap(m_vertexBuffer.Get(), 0);
}

void Model::Draw(const AppContext& appContext)
{
	const auto& view = appContext.m_viewMat;
	const auto& proj = appContext.m_projMat;
	const auto& dxMat = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);

	auto world = glm::mat4(1.0f);
	auto wv = view * world;
	auto wvp = dxMat * proj * view * world;
	auto wvit = glm::mat3(view * world);
	wvit = glm::inverse(wvit);
	wvit = glm::transpose(wvit);

	// Set viewport
	D3D11_VIEWPORT vp;
	vp.Width = float(appContext.m_screenWidth);
	vp.Height = float(appContext.m_screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);
	ID3D11RenderTargetView* rtvs[] = { appContext.m_renderTargetView.Get() };
	m_context->OMSetRenderTargets(1, rtvs,
		appContext.m_depthStencilView.Get()
	);

	m_context->OMSetDepthStencilState(appContext.m_defaultDSS.Get(), 0x00);

	// Setup input assembler
	{
		UINT strides[] = { sizeof(Vertex) };
		UINT offsets[] = { 0 };
		m_context->IASetInputLayout(appContext.m_mmdInputLayout.Get());
		ID3D11Buffer* vbs[] = { m_vertexBuffer.Get() };
		m_context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		m_context->IASetIndexBuffer(m_indexBuffer.Get(), m_indexBufferFormat, 0);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Draw model

	// Setup vertex shader
	{
		MMDVertexShaderCB vsCB;
		vsCB.m_wv = wv;
		vsCB.m_wvp = wvp;
		m_context->UpdateSubresource(m_mmdVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

		// Vertex shader
		m_context->VSSetShader(appContext.m_mmdVS.Get(), nullptr, 0);
		ID3D11Buffer* cbs[] = { m_mmdVSConstantBuffer.Get() };
		m_context->VSSetConstantBuffers(0, 1, cbs);
	}
	size_t subMeshCount = m_mmdModel->GetSubMeshCount();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (mat.m_mmdMat.m_alpha == 0)
		{
			continue;
		}

		// Pixel shader
		m_context->PSSetShader(appContext.m_mmdPS.Get(), nullptr, 0);

		MMDPixelShaderCB psCB;
		psCB.m_alpha = mmdMat.m_alpha;
		psCB.m_diffuse = mmdMat.m_diffuse;
		psCB.m_ambient = mmdMat.m_ambient;
		psCB.m_specular = mmdMat.m_specular;
		psCB.m_specularPower = mmdMat.m_specularPower;

		if (mat.m_texture.m_texture)
		{
			if (!mat.m_texture.m_hasAlpha)
			{
				// Use Material Alpha
				psCB.m_textureModes.x = 1;
			}
			else
			{
				// Use Material Alpha * Texture Alpha
				psCB.m_textureModes.x = 2;
			}
			psCB.m_texMulFactor = mmdMat.m_textureMulFactor;
			psCB.m_texAddFactor = mmdMat.m_textureAddFactor;
			ID3D11ShaderResourceView* views[] = { mat.m_texture.m_textureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_textureSampler.Get() };
			m_context->PSSetShaderResources(0, 1, views);
			m_context->PSSetSamplers(0, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.x = 0;
			ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
			m_context->PSSetShaderResources(0, 1, views);
			m_context->PSSetSamplers(0, 1, samplers);
		}

		if (mat.m_toonTexture.m_texture)
		{
			psCB.m_textureModes.y = 1;
			psCB.m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
			psCB.m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
			ID3D11ShaderResourceView* views[] = { mat.m_toonTexture.m_textureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_toonTextureSampler.Get() };
			m_context->PSSetShaderResources(1, 1, views);
			m_context->PSSetSamplers(1, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.y = 0;
			ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
			m_context->PSSetShaderResources(1, 1, views);
			m_context->PSSetSamplers(1, 1, samplers);
		}

		if (mat.m_spTexture.m_texture)
		{
			if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
			{
				psCB.m_textureModes.z = 1;
			}
			else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
			{
				psCB.m_textureModes.z = 2;
			}
			psCB.m_sphereTexMulFactor = mmdMat.m_spTextureMulFactor;
			psCB.m_sphereTexAddFactor = mmdMat.m_spTextureAddFactor;
			ID3D11ShaderResourceView* views[] = { mat.m_spTexture.m_textureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_sphereTextureSampler.Get() };
			m_context->PSSetShaderResources(2, 1, views);
			m_context->PSSetSamplers(2, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.z = 0;
			ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
			m_context->PSSetShaderResources(2, 1, views);
			m_context->PSSetSamplers(2, 1, samplers);
		}

		psCB.m_lightColor = appContext.m_lightColor;
		glm::vec3 lightDir = appContext.m_lightDir;
		glm::mat3 viewMat = glm::mat3(appContext.m_viewMat);
		lightDir = viewMat * lightDir;
		psCB.m_lightDir = lightDir;

		m_context->UpdateSubresource(m_mmdPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);
		ID3D11Buffer* pscbs[] = { m_mmdPSConstantBuffer.Get() };
		m_context->PSSetConstantBuffers(1, 1, pscbs);

		if (mmdMat.m_bothFace)
		{
			m_context->RSSetState(appContext.m_mmdBothFaceRS.Get());
		}
		else
		{
			m_context->RSSetState(appContext.m_mmdFrontFaceRS.Get());
		}

		m_context->OMSetBlendState(appContext.m_mmdBlendState.Get(), nullptr, 0xffffffff);

		m_context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}

	{
		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
		ID3D11SamplerState* samplers[] = { nullptr, nullptr, nullptr };
		m_context->PSSetShaderResources(0, 3, views);
		m_context->PSSetSamplers(0, 3, samplers); 
	}

	// Draw edge

	// Setup input assembler
	{
		m_context->IASetInputLayout(appContext.m_mmdEdgeInputLayout.Get());
	}

	// Setup vertex shader (VSData)
	{
		MMDEdgeVertexShaderCB vsCB;
		vsCB.m_wv = wv;
		vsCB.m_wvp = wvp;
		vsCB.m_screenSize = glm::vec2(float(appContext.m_screenWidth), float(appContext.m_screenHeight));
		m_context->UpdateSubresource(m_mmdEdgeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

		// Vertex shader
		m_context->VSSetShader(appContext.m_mmdEdgeVS.Get(), nullptr, 0);
		ID3D11Buffer* cbs[] = { m_mmdEdgeVSConstantBuffer.Get() };
		m_context->VSSetConstantBuffers(0, 1, cbs);
	}

	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (!mmdMat.m_edgeFlag)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		// Edge size constant buffer
		{
			MMDEdgeSizeVertexShaderCB vsCB;
			vsCB.m_edgeSize = mmdMat.m_edgeSize;
			m_context->UpdateSubresource(m_mmdEdgeSizeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

			ID3D11Buffer* cbs[] = { m_mmdEdgeSizeVSConstantBuffer.Get() };
			m_context->VSSetConstantBuffers(1, 1, cbs);
		}

		// Pixel shader
		m_context->PSSetShader(appContext.m_mmdEdgePS.Get(), nullptr, 0);
		{
			MMDEdgePixelShaderCB psCB;
			psCB.m_edgeColor = mmdMat.m_edgeColor;
			m_context->UpdateSubresource(m_mmdEdgePSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

			ID3D11Buffer* pscbs[] = { m_mmdEdgePSConstantBuffer.Get() };
			m_context->PSSetConstantBuffers(2, 1, pscbs);
		}

		m_context->RSSetState(appContext.m_mmdEdgeRS.Get());

		m_context->OMSetBlendState(appContext.m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

		m_context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}

	// Draw ground shadow

	// Setup input assembler
	{
		m_context->IASetInputLayout(appContext.m_mmdGroundShadowInputLayout.Get());
	}

	// Setup vertex shader (VSData)
	{
		auto plane = glm::vec4(0, 1, 0, 0);
		auto light = -appContext.m_lightDir;
		auto shadow = glm::mat4(1);

		shadow[0][0] = plane.y * light.y + plane.z * light.z;
		shadow[0][1] = -plane.x * light.y;
		shadow[0][2] = -plane.x * light.z;
		shadow[0][3] = 0;

		shadow[1][0] = -plane.y * light.x;
		shadow[1][1] = plane.x * light.x + plane.z * light.z;
		shadow[1][2] = -plane.y * light.z;
		shadow[1][3] = 0;

		shadow[2][0] = -plane.z * light.x;
		shadow[2][1] = -plane.z * light.y;
		shadow[2][2] = plane.x * light.x + plane.y * light.y;
		shadow[2][3] = 0;

		shadow[3][0] = -plane.w * light.x;
		shadow[3][1] = -plane.w * light.y;
		shadow[3][2] = -plane.w * light.z;
		shadow[3][3] = plane.x * light.x + plane.y * light.y + plane.z * light.z;

		auto wsvp = dxMat * proj * view * shadow * world;

		MMDGroundShadowVertexShaderCB vsCB;
		vsCB.m_wvp = wsvp;
		m_context->UpdateSubresource(m_mmdGroundShadowVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

		// Vertex shader
		m_context->VSSetShader(appContext.m_mmdGroundShadowVS.Get(), nullptr, 0);
		ID3D11Buffer* cbs[] = { m_mmdGroundShadowVSConstantBuffer.Get() };
		m_context->VSSetConstantBuffers(0, 1, cbs);
	}

	// Setup state
	{
		m_context->RSSetState(appContext.m_mmdGroundShadowRS.Get());
		m_context->OMSetBlendState(appContext.m_mmdGroundShadowBlendState.Get(), nullptr, 0xffffffff);
		m_context->OMSetDepthStencilState(appContext.m_mmdGroundShadowDSS.Get(), 0x01);
	}

	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (!mmdMat.m_groundShadow)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		// Pixel shader
		m_context->PSSetShader(appContext.m_mmdGroundShadowPS.Get(), nullptr, 0);
		{
			MMDGroundShadowPixelShaderCB psCB;
			psCB.m_shadowColor = glm::vec4(0.4f, 0.2f, 0.2f, 0.7f);
			m_context->UpdateSubresource(m_mmdGroundShadowPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

			ID3D11Buffer* pscbs[] = { m_mmdGroundShadowPSConstantBuffer.Get() };
			m_context->PSSetConstantBuffers(1, 1, pscbs);
		}

		m_context->OMSetBlendState(appContext.m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

		m_context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}
}

class App
{
public:
	App(HWND hwnd);
	bool Run(const std::vector<std::string>& args);
	void Exit();
	void Resize(int w, int h);

private:
	void MainLoop();
	bool CreateRenderTarget();

private:
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	HWND				m_hwnd;
	std::thread			m_mainThread;
	std::atomic<bool>	m_runable;
	int					m_width;
	int					m_height;

	UINT				m_multiSampleCount = 1;
	UINT				m_multiSampleQuality = 0;

	ComPtr<ID3D11Device>	m_device;
	ComPtr<IDXGISwapChain>	m_swapChain;
	ComPtr<ID3D11DeviceContext>	m_deviceContext;
	ComPtr<ID3D11RenderTargetView>	m_renderTargetView;
	ComPtr<ID3D11Texture2D>			m_depthStencilTex;
	ComPtr<ID3D11DepthStencilView>	m_depthStencilView;

	AppContext			m_appContext;
	std::vector<Model>	m_models;

};

App::App(HWND hwnd)
	: m_hwnd(hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
}

bool App::Run(const std::vector<std::string>& args)
{
	// Parse args
	Input currentInput;
	std::vector<Input> inputModels;
	for (auto argIt = args.begin(); argIt != args.end(); ++argIt)
	{
		const auto& arg = (*argIt);
		if (arg == "-model")
		{
			if (!currentInput.m_modelPath.empty())
			{
				inputModels.emplace_back(currentInput);
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput = Input();
			currentInput.m_modelPath = (*argIt);
		}
		else if (arg == "-vmd")
		{
			if (currentInput.m_modelPath.empty())
			{
				Usage();
				return false;
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput.m_vmdPaths.push_back((*argIt));
		}
	}
	if (!currentInput.m_modelPath.empty())
	{
		inputModels.emplace_back(currentInput);
	}

	// Setup DirectX 11

	UINT createDeviceFlags = 0;
#if defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	HRESULT hr = D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, 1,
		D3D11_SDK_VERSION, &m_device, &featureLevel, &m_deviceContext
	);
	if (FAILED(hr))
	{
		return false;
	}

	m_multiSampleCount = 4;
	UINT quality;
	hr = m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &quality);
	if (FAILED(hr))
	{
		return false;
	}
	m_multiSampleQuality = quality - 1;

	ComPtr<IDXGIDevice1> dxgi;
	hr = m_device.CopyTo<IDXGIDevice1>(&dxgi);
	if (FAILED(hr))
	{
		return false;
	}
	ComPtr<IDXGIAdapter> adapter;
	hr = dxgi->GetAdapter(&adapter);
	if (FAILED(hr))
	{
		return false;
	}
	ComPtr<IDXGIFactory> dxgiFactory;
	hr = adapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);
	if (FAILED(hr))
	{
		return false;
	}

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 1;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = m_hwnd;
	sd.SampleDesc.Count = m_multiSampleCount;
	sd.SampleDesc.Quality = m_multiSampleQuality;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	hr = dxgiFactory->CreateSwapChain(m_device.Get(), &sd, &m_swapChain);
	if (FAILED(hr))
	{
		return false;
	}

	if (!CreateRenderTarget())
	{
		return false;
	}

	m_appContext.m_multiSampleCount = m_multiSampleCount;
	m_appContext.m_multiSampleQuality = m_multiSampleQuality;
	if (!m_appContext.Setup(m_device))
	{
		return false;
	}

	for (const auto& input : inputModels)
	{
		// Load MMD model
		Model model;
		auto ext = saba::PathUtil::GetExt(input.m_modelPath);
		if (ext == "pmd")
		{
			auto pmdModel = std::make_unique<saba::PMDModel>();
			if (!pmdModel->Load(input.m_modelPath, m_appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmd file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmdModel);
		}
		else if (ext == "pmx")
		{
			auto pmxModel = std::make_unique<saba::PMXModel>();
			if (!pmxModel->Load(input.m_modelPath, m_appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmx file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmxModel);
		}
		else
		{
			std::cout << "Unknown file type. [" << ext << "]\n";
			return false;
		}
		model.m_mmdModel->InitializeAnimation();

		// Load VMD animation
		auto vmdAnim = std::make_unique<saba::VMDAnimation>();
		if (!vmdAnim->Create(model.m_mmdModel))
		{
			std::cout << "Failed to create VMDAnimation.\n";
			return false;
		}
		for (const auto& vmdPath : input.m_vmdPaths)
		{
			saba::VMDFile vmdFile;
			if (!saba::ReadVMDFile(&vmdFile, vmdPath.c_str()))
			{
				std::cout << "Failed to read VMD file.\n";
				return false;
			}
			if (!vmdAnim->Add(vmdFile))
			{
				std::cout << "Failed to add VMDAnimation.\n";
				return false;
			}
			if (!vmdFile.m_cameras.empty())
			{
				auto vmdCamAnim = std::make_unique<saba::VMDCameraAnimation>();
				if (!vmdCamAnim->Create(vmdFile))
				{
					std::cout << "Failed to create VMDCameraAnimation.\n";
				}
				m_appContext.m_vmdCameraAnim = std::move(vmdCamAnim);
			}
		}
		vmdAnim->SyncPhysics(0.0f);

		model.m_vmdAnim = std::move(vmdAnim);

		model.Setup(m_appContext);

		m_models.emplace_back(std::move(model));
	}

	m_runable = true;
	m_mainThread = std::thread([this]() { this->MainLoop(); });
	return true;
}

void App::Exit()
{
	if (m_runable)
	{
		m_runable = false;
		m_mainThread.join();
	}
}

void App::Resize(int w, int h)
{
	m_width = w;
	m_height = h;
}

void App::MainLoop()
{
	double fpsTime = saba::GetTime();
	int fpsFrame = 0;
	double saveTime = saba::GetTime();
	while (m_runable)
	{
		// Check render target size
		DXGI_SWAP_CHAIN_DESC sd;
		m_swapChain->GetDesc(&sd);
		if (sd.BufferDesc.Width != m_width || sd.BufferDesc.Height != m_height)
		{
			m_renderTargetView.Reset();
			m_appContext.m_renderTargetView.Reset();
			m_depthStencilTex.Reset();
			m_depthStencilView.Reset();
			m_appContext.m_depthStencilView.Reset();

			m_swapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		m_appContext.m_renderTargetView = m_renderTargetView;
		m_appContext.m_depthStencilView = m_depthStencilView;

		// Set viewport
		D3D11_VIEWPORT vp;
		vp.Width = float(m_width);
		vp.Height = float(m_height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_deviceContext->RSSetViewports(1, &vp);

		// Clear target
		ID3D11RenderTargetView* rtvs[] = { m_renderTargetView.Get() };
		float clearColor[] = { 1.0f, 0.8f, 0.75f, 1 };
		m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
		m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(),
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0
		);

		// Set output
		m_deviceContext->OMSetRenderTargets(1, rtvs, m_depthStencilView.Get());

		double time = saba::GetTime();
		double elapsed = time - saveTime;
		if (elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		saveTime = time;
		m_appContext.m_elapsed = float(elapsed);
		m_appContext.m_animTime += float(elapsed);

		m_appContext.m_screenWidth = m_width;
		m_appContext.m_screenHeight = m_height;

		// Setup camera
		if (m_appContext.m_vmdCameraAnim)
		{
			m_appContext.m_vmdCameraAnim->Evaluate(m_appContext.m_animTime * 30.0f);
			const auto mmdCam = m_appContext.m_vmdCameraAnim->GetCamera();
			saba::MMDLookAtCamera lookAtCam(mmdCam);
			m_appContext.m_viewMat = glm::lookAt(lookAtCam.m_eye, lookAtCam.m_center, lookAtCam.m_up);
			m_appContext.m_projMat = glm::perspectiveFovRH(mmdCam.m_fov, float(m_width), float(m_height), 1.0f, 10000.0f);
		}
		else
		{
			m_appContext.m_viewMat = glm::lookAt(glm::vec3(0, 10, 50), glm::vec3(0, 10, 0), glm::vec3(0, 1, 0));
			m_appContext.m_projMat = glm::perspectiveFovRH(glm::radians(30.0f), float(m_width), float(m_height), 1.0f, 10000.0f);
		}

		for (auto& model : m_models)
		{
			// Update Animation
			model.UpdateAnimation(m_appContext);

			// Update Vertices
			model.Update(m_appContext);

			// Draw
			model.Draw(m_appContext);

			ComPtr<ID3D11CommandList> cmdList;
			HRESULT hr = model.m_context->FinishCommandList(FALSE, &cmdList);

			m_deviceContext->ExecuteCommandList(cmdList.Get(), FALSE);
		}

		m_swapChain->Present(0, 0);

		//FPS
		{
			fpsFrame++;
			double time = saba::GetTime();
			double deltaTime = time - fpsTime;
			if (deltaTime > 1.0)
			{
				double fps = double(fpsFrame) / deltaTime;
				std::cout << fps << " fps\n";
				fpsFrame = 0;
				fpsTime = time;
			}
		}
	}
}

bool App::CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	m_swapChain->GetDesc(&sd);

	ComPtr<ID3D11Texture2D> backBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = sd.BufferDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	HRESULT hr = m_device->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &m_renderTargetView);
	if (FAILED(hr))
	{
		return false;
	}

	// DepthStencil
	D3D11_TEXTURE2D_DESC tex2dDesc = {};
	tex2dDesc.Width = sd.BufferDesc.Width;
	tex2dDesc.Height = sd.BufferDesc.Height;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	tex2dDesc.SampleDesc.Count = m_multiSampleCount;
	tex2dDesc.SampleDesc.Quality = m_multiSampleQuality;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	tex2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.MiscFlags = 0;
	hr = m_device->CreateTexture2D(&tex2dDesc, nullptr, &m_depthStencilTex);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = m_device->CreateDepthStencilView(m_depthStencilTex.Get(), &dsvDesc, &m_depthStencilView);
	if (FAILED(hr))
	{
		return false;
	}
	return true;
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	App* app = (App*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_DESTROY:
		app->Exit();
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (app != nullptr)
		{
			app->Resize((UINT)LOWORD(lparam), (UINT)HIWORD(lparam));
		}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		Usage();
		return 1;
	}

	std::vector<std::string> args(argc);
	{
		WCHAR* cmdline = GetCommandLineW();
		int wArgc;
		WCHAR** wArgs = CommandLineToArgvW(cmdline, &wArgc);
		for (int i = 0; i < argc; i++)
		{
			args[i] = saba::ToUtf8String(wArgs[i]);
		}
	}

	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0,
		0,
		GetModuleHandle(nullptr),
		NULL,
		LoadCursor(NULL, IDC_ARROW),
		NULL,
		nullptr,
		_T("simple mmd viewer dx11"),
		NULL
	};
	RegisterClassEx(&wc);
	RECT rect = { 0, 0, 1280, 800 };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindow(
		_T("simple mmd viewer dx11"),
		_T("simple_mmd_viewer_dx11"),
		WS_OVERLAPPEDWINDOW,
		100, 100,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL
	);

	auto app = std::make_unique<App>(hwnd);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(app.get()));
	if (!app->Run(args))
	{
		return 1;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg;

	while (1)
	{
		auto ret = GetMessage(&msg, NULL, 0, 0);
		if (ret > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (ret == 0)
		{
			break;
		}
	}

	app.reset();

	return int(msg.wParam);
}
