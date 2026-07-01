#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
constexpr wchar_t kWindowClassName[] = L"DSForwardRendererWindow";
constexpr UINT kInitialWidth = 1280;
constexpr UINT kInitialHeight = 720;
constexpr UINT kShadowMapSize = 2048;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

struct FrameConstants
{
    XMFLOAT3 cameraPosition;
    float exposure;
    XMFLOAT3 lightDirection;
    float lightIntensity;
    XMFLOAT3 lightColor;
    float ambientIntensity;
    XMFLOAT2 shadowMapTexelSize;
    XMFLOAT2 framePadding;
};

struct ObjectConstants
{
    XMMATRIX world;
    XMMATRIX worldViewProjection;
    XMMATRIX lightWorldViewProjection;
    XMFLOAT3 albedo;
    float metallic;
    float roughness;
    XMFLOAT3 objectPadding;
};

struct Mesh
{
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount = 0;
};

struct Material
{
    XMFLOAT3 albedo;
    float metallic;
    float roughness;
};

struct RenderItem
{
    Mesh* mesh = nullptr;
    XMMATRIX world = XMMatrixIdentity();
    Material material = {};
    bool castsShadow = true;
};

class ForwardRenderer
{
public:
    void Initialize(HWND hwnd, UINT width, UINT height)
    {
        hwnd_ = hwnd;
        CreateDeviceAndSwapChain(width, height);
        CreateRenderTargets(width, height);
        CreateShadowMap();
        CreateShaders();
        CreateGeometry();
        CreateStates();
    }

    void Resize(UINT width, UINT height)
    {
        if (!device_ || width == 0 || height == 0)
        {
            return;
        }

        context_->OMSetRenderTargets(0, nullptr, nullptr);
        renderTargetView_.Reset();
        depthStencilView_.Reset();
        depthStencilBuffer_.Reset();

        ThrowIfFailed(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
        CreateRenderTargets(width, height);
    }

    void Render(float timeSeconds)
    {
        const XMVECTOR cameraPosition = XMVectorSet(0.0f, 2.7f, -7.0f, 1.0f);
        const XMVECTOR cameraTarget = XMVectorSet(0.0f, 0.2f, 0.0f, 1.0f);
        const XMMATRIX view = XMMatrixLookAtLH(cameraPosition, cameraTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        const float aspect = static_cast<float>(std::max<UINT>(viewportWidth_, 1)) /
            static_cast<float>(std::max<UINT>(viewportHeight_, 1));
        const XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(55.0f), aspect, 0.1f, 100.0f);

        const XMVECTOR lightDirection = XMVector3Normalize(XMVectorSet(-0.42f, -0.82f, 0.38f, 0.0f));
        const XMVECTOR lightTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        const XMVECTOR lightPosition = XMVectorSubtract(lightTarget, XMVectorScale(lightDirection, 11.0f));
        const XMMATRIX lightView = XMMatrixLookAtLH(lightPosition, lightTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        const XMMATRIX lightProjection = XMMatrixOrthographicLH(15.0f, 15.0f, 0.1f, 30.0f);
        const XMMATRIX lightViewProjection = lightView * lightProjection;

        XMFLOAT3 cameraPositionFloat = {};
        XMFLOAT3 lightDirectionFloat = {};
        XMStoreFloat3(&cameraPositionFloat, cameraPosition);
        XMStoreFloat3(&lightDirectionFloat, lightDirection);

        FrameConstants frameConstants = {};
        frameConstants.cameraPosition = cameraPositionFloat;
        frameConstants.exposure = 1.05f;
        frameConstants.lightDirection = lightDirectionFloat;
        frameConstants.lightIntensity = 7.5f;
        frameConstants.lightColor = XMFLOAT3(1.0f, 0.95f, 0.86f);
        frameConstants.ambientIntensity = 0.045f;
        frameConstants.shadowMapTexelSize = XMFLOAT2(1.0f / kShadowMapSize, 1.0f / kShadowMapSize);
        context_->UpdateSubresource(frameConstantBuffer_.Get(), 0, nullptr, &frameConstants, 0, 0);

        const std::vector<RenderItem> scene = BuildScene(timeSeconds);
        RenderShadowPass(scene, lightViewProjection);
        RenderForwardPass(scene, view * projection, lightViewProjection);
    }

private:
    static void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error("Direct3D call failed.");
        }
    }

    void CreateDeviceAndSwapChain(UINT width, UINT height)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd_;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createFlags = 0;
#if defined(_DEBUG)
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        const D3D_FEATURE_LEVEL requestedLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        D3D_FEATURE_LEVEL createdLevel = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            requestedLevels,
            ARRAYSIZE(requestedLevels),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            swapChain_.GetAddressOf(),
            device_.GetAddressOf(),
            &createdLevel,
            context_.GetAddressOf());

#if defined(_DEBUG)
        if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
        {
            createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                createFlags,
                requestedLevels,
                ARRAYSIZE(requestedLevels),
                D3D11_SDK_VERSION,
                &swapChainDesc,
                swapChain_.GetAddressOf(),
                device_.GetAddressOf(),
                &createdLevel,
                context_.GetAddressOf());
        }
#endif

        ThrowIfFailed(hr);
    }

    void CreateRenderTargets(UINT width, UINT height)
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        ThrowIfFailed(swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
        ThrowIfFailed(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf()));

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ThrowIfFailed(device_->CreateTexture2D(&depthDesc, nullptr, depthStencilBuffer_.GetAddressOf()));
        ThrowIfFailed(device_->CreateDepthStencilView(depthStencilBuffer_.Get(), nullptr, depthStencilView_.GetAddressOf()));

        viewportWidth_ = width;
        viewportHeight_ = height;
        viewport_.TopLeftX = 0.0f;
        viewport_.TopLeftY = 0.0f;
        viewport_.Width = static_cast<float>(width);
        viewport_.Height = static_cast<float>(height);
        viewport_.MinDepth = 0.0f;
        viewport_.MaxDepth = 1.0f;
    }

    void CreateShadowMap()
    {
        D3D11_TEXTURE2D_DESC shadowDesc = {};
        shadowDesc.Width = kShadowMapSize;
        shadowDesc.Height = kShadowMapSize;
        shadowDesc.MipLevels = 1;
        shadowDesc.ArraySize = 1;
        shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        shadowDesc.SampleDesc.Count = 1;
        shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        ThrowIfFailed(device_->CreateTexture2D(&shadowDesc, nullptr, shadowMap_.GetAddressOf()));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        ThrowIfFailed(device_->CreateDepthStencilView(shadowMap_.Get(), &dsvDesc, shadowDepthView_.GetAddressOf()));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        ThrowIfFailed(device_->CreateShaderResourceView(shadowMap_.Get(), &srvDesc, shadowResourceView_.GetAddressOf()));

        shadowViewport_.TopLeftX = 0.0f;
        shadowViewport_.TopLeftY = 0.0f;
        shadowViewport_.Width = static_cast<float>(kShadowMapSize);
        shadowViewport_.Height = static_cast<float>(kShadowMapSize);
        shadowViewport_.MinDepth = 0.0f;
        shadowViewport_.MaxDepth = 1.0f;
    }

    void CreateShaders()
    {
        constexpr char shaderSource[] = R"(
cbuffer FrameConstants : register(b0)
{
    float3 cameraPosition;
    float exposure;
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float ambientIntensity;
    float2 shadowMapTexelSize;
    float2 framePadding;
};

cbuffer ObjectConstants : register(b1)
{
    matrix world;
    matrix worldViewProjection;
    matrix lightWorldViewProjection;
    float3 albedo;
    float metallic;
    float roughness;
    float3 objectPadding;
};

Texture2D shadowMap : register(t0);
SamplerComparisonState shadowSampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 normal : NORMAL;
    float4 lightPosition : TEXCOORD1;
};

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.lightPosition = mul(float4(input.position, 1.0f), lightWorldViewProjection);
    return output;
}

float4 DepthVSMain(VertexInput input) : SV_POSITION
{
    return mul(float4(input.position, 1.0f), lightWorldViewProjection);
}

static const float PI = 3.14159265f;

float DistributionGGX(float3 normal, float3 halfway, float surfaceRoughness)
{
    float alpha = surfaceRoughness * surfaceRoughness;
    float alpha2 = alpha * alpha;
    float nDotH = saturate(dot(normal, halfway));
    float nDotH2 = nDotH * nDotH;
    float denom = nDotH2 * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / max(PI * denom * denom, 0.00001f);
}

float GeometrySchlickGGX(float nDotV, float surfaceRoughness)
{
    float r = surfaceRoughness + 1.0f;
    float k = (r * r) / 8.0f;
    return nDotV / max(nDotV * (1.0f - k) + k, 0.00001f);
}

float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float surfaceRoughness)
{
    float nDotV = saturate(dot(normal, viewDir));
    float nDotL = saturate(dot(normal, lightDir));
    return GeometrySchlickGGX(nDotV, surfaceRoughness) * GeometrySchlickGGX(nDotL, surfaceRoughness);
}

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - saturate(cosTheta), 5.0f);
}

float ShadowVisibility(float4 lightClipPosition)
{
    float3 projected = lightClipPosition.xyz / lightClipPosition.w;
    float2 uv = float2(projected.x * 0.5f + 0.5f, -projected.y * 0.5f + 0.5f);
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || projected.z < 0.0f || projected.z > 1.0f)
    {
        return 1.0f;
    }

    float depth = projected.z - 0.0015f;
    float visibility = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            visibility += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x, y) * shadowMapTexelSize, depth);
        }
    }
    return visibility / 9.0f;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float3 baseColor = pow(saturate(albedo), 2.2f);
    float surfaceMetallic = saturate(metallic);
    float surfaceRoughness = clamp(roughness, 0.04f, 1.0f);

    float3 n = normalize(input.normal);
    float3 v = normalize(cameraPosition - input.worldPosition);
    float3 l = normalize(-lightDirection);
    float3 h = normalize(v + l);

    float nDotL = saturate(dot(n, l));
    float nDotV = saturate(dot(n, v));
    float3 radiance = lightColor * lightIntensity;

    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, surfaceMetallic);
    float3 fresnel = FresnelSchlick(saturate(dot(h, v)), f0);
    float distribution = DistributionGGX(n, h, surfaceRoughness);
    float geometry = GeometrySmith(n, v, l, surfaceRoughness);

    float3 specular = distribution * geometry * fresnel / max(4.0f * nDotV * nDotL, 0.00001f);
    float3 diffuse = (1.0f - fresnel) * (1.0f - surfaceMetallic) * baseColor / PI;
    float visibility = ShadowVisibility(input.lightPosition);
    float3 direct = (diffuse + specular) * radiance * nDotL * visibility;

    float3 ambient = baseColor * ambientIntensity * (1.0f - surfaceMetallic * 0.65f);
    float3 color = ambient + direct;
    color = float3(1.0f, 1.0f, 1.0f) - exp(-color * exposure);
    color = pow(saturate(color), 1.0f / 2.2f);
    return float4(color, 1.0f);
}
)";

        ComPtr<ID3DBlob> forwardVertexBlob;
        ComPtr<ID3DBlob> depthVertexBlob;
        ComPtr<ID3DBlob> pixelBlob;
        CompileShader(shaderSource, "VSMain", "vs_5_0", forwardVertexBlob.GetAddressOf());
        CompileShader(shaderSource, "DepthVSMain", "vs_5_0", depthVertexBlob.GetAddressOf());
        CompileShader(shaderSource, "PSMain", "ps_5_0", pixelBlob.GetAddressOf());

        ThrowIfFailed(device_->CreateVertexShader(
            forwardVertexBlob->GetBufferPointer(),
            forwardVertexBlob->GetBufferSize(),
            nullptr,
            forwardVertexShader_.GetAddressOf()));
        ThrowIfFailed(device_->CreateVertexShader(
            depthVertexBlob->GetBufferPointer(),
            depthVertexBlob->GetBufferSize(),
            nullptr,
            depthVertexShader_.GetAddressOf()));
        ThrowIfFailed(device_->CreatePixelShader(
            pixelBlob->GetBufferPointer(),
            pixelBlob->GetBufferSize(),
            nullptr,
            pixelShader_.GetAddressOf()));

        const D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        ThrowIfFailed(device_->CreateInputLayout(
            layout,
            ARRAYSIZE(layout),
            forwardVertexBlob->GetBufferPointer(),
            forwardVertexBlob->GetBufferSize(),
            inputLayout_.GetAddressOf()));
    }

    void CompileShader(const char* source, const char* entryPoint, const char* target, ID3DBlob** shaderBlob) const
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ComPtr<ID3DBlob> errors;
        const HRESULT hr = D3DCompile(
            source,
            strlen(source),
            nullptr,
            nullptr,
            nullptr,
            entryPoint,
            target,
            flags,
            0,
            shaderBlob,
            errors.GetAddressOf());

        if (FAILED(hr))
        {
            if (errors)
            {
                OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
            }
            ThrowIfFailed(hr);
        }
    }

    void CreateGeometry()
    {
        CreateCubeMesh(cubeMesh_);
        CreatePlaneMesh(planeMesh_);
        CreateSphereMesh(sphereMesh_, 32, 16);

        D3D11_BUFFER_DESC frameDesc = {};
        frameDesc.ByteWidth = sizeof(FrameConstants);
        frameDesc.Usage = D3D11_USAGE_DEFAULT;
        frameDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ThrowIfFailed(device_->CreateBuffer(&frameDesc, nullptr, frameConstantBuffer_.GetAddressOf()));

        D3D11_BUFFER_DESC objectDesc = {};
        objectDesc.ByteWidth = sizeof(ObjectConstants);
        objectDesc.Usage = D3D11_USAGE_DEFAULT;
        objectDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ThrowIfFailed(device_->CreateBuffer(&objectDesc, nullptr, objectConstantBuffer_.GetAddressOf()));
    }

    void CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, Mesh& mesh)
    {
        D3D11_BUFFER_DESC vertexDesc = {};
        vertexDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
        vertexDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices.data();
        ThrowIfFailed(device_->CreateBuffer(&vertexDesc, &vertexData, mesh.vertexBuffer.GetAddressOf()));

        D3D11_BUFFER_DESC indexDesc = {};
        indexDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint16_t));
        indexDesc.Usage = D3D11_USAGE_DEFAULT;
        indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = indices.data();
        ThrowIfFailed(device_->CreateBuffer(&indexDesc, &indexData, mesh.indexBuffer.GetAddressOf()));
        mesh.indexCount = static_cast<UINT>(indices.size());
    }

    void CreateCubeMesh(Mesh& mesh)
    {
        const std::vector<Vertex> vertices = {
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }
        };

        const std::vector<uint16_t> indices = {
             0,  1,  2,  0,  2,  3,
             4,  5,  6,  4,  6,  7,
             8,  9, 10,  8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23
        };

        CreateMesh(vertices, indices, mesh);
    }

    void CreatePlaneMesh(Mesh& mesh)
    {
        constexpr float size = 8.0f;
        const std::vector<Vertex> vertices = {
            { XMFLOAT3(-size, 0.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(-size, 0.0f,  size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3( size, 0.0f,  size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3( size, 0.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) }
        };
        const std::vector<uint16_t> indices = { 0, 1, 2, 0, 2, 3 };
        CreateMesh(vertices, indices, mesh);
    }

    void CreateSphereMesh(Mesh& mesh, UINT slices, UINT stacks)
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        vertices.reserve((slices + 1) * (stacks + 1));

        for (UINT stack = 0; stack <= stacks; ++stack)
        {
            const float v = static_cast<float>(stack) / static_cast<float>(stacks);
            const float phi = v * XM_PI;
            for (UINT slice = 0; slice <= slices; ++slice)
            {
                const float u = static_cast<float>(slice) / static_cast<float>(slices);
                const float theta = u * XM_2PI;
                const float x = std::sinf(phi) * std::cosf(theta);
                const float y = std::cosf(phi);
                const float z = std::sinf(phi) * std::sinf(theta);
                vertices.push_back({ XMFLOAT3(x, y, z), XMFLOAT3(x, y, z) });
            }
        }

        for (UINT stack = 0; stack < stacks; ++stack)
        {
            for (UINT slice = 0; slice < slices; ++slice)
            {
                const uint16_t a = static_cast<uint16_t>(stack * (slices + 1) + slice);
                const uint16_t b = static_cast<uint16_t>(a + slices + 1);
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(static_cast<uint16_t>(a + 1));
                indices.push_back(static_cast<uint16_t>(a + 1));
                indices.push_back(b);
                indices.push_back(static_cast<uint16_t>(b + 1));
            }
        }

        CreateMesh(vertices, indices, mesh);
    }

    void CreateStates()
    {
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = TRUE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
        ThrowIfFailed(device_->CreateDepthStencilState(&depthDesc, depthStencilState_.GetAddressOf()));

        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        ThrowIfFailed(device_->CreateRasterizerState(&rasterDesc, rasterizerState_.GetAddressOf()));

        rasterDesc.DepthBias = 1600;
        rasterDesc.SlopeScaledDepthBias = 1.5f;
        rasterDesc.DepthBiasClamp = 0.0f;
        ThrowIfFailed(device_->CreateRasterizerState(&rasterDesc, shadowRasterizerState_.GetAddressOf()));

        D3D11_SAMPLER_DESC shadowSamplerDesc = {};
        shadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        shadowSamplerDesc.BorderColor[0] = 1.0f;
        shadowSamplerDesc.BorderColor[1] = 1.0f;
        shadowSamplerDesc.BorderColor[2] = 1.0f;
        shadowSamplerDesc.BorderColor[3] = 1.0f;
        shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        shadowSamplerDesc.MinLOD = 0.0f;
        shadowSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        ThrowIfFailed(device_->CreateSamplerState(&shadowSamplerDesc, shadowSampler_.GetAddressOf()));
    }

    std::vector<RenderItem> BuildScene(float timeSeconds)
    {
        std::vector<RenderItem> scene;
        scene.reserve(7);

        scene.push_back({
            &planeMesh_,
            XMMatrixTranslation(0.0f, -1.25f, 0.0f),
            { XMFLOAT3(0.58f, 0.61f, 0.56f), 0.0f, 0.68f },
            false
        });

        scene.push_back({
            &cubeMesh_,
            XMMatrixScaling(0.85f, 0.85f, 0.85f) *
                XMMatrixRotationRollPitchYaw(timeSeconds * 0.7f, timeSeconds * 1.1f, 0.0f) *
                XMMatrixTranslation(0.0f, -0.25f, 0.0f),
            { XMFLOAT3(0.86f, 0.17f, 0.13f), 0.0f, 0.36f },
            true
        });

        scene.push_back({
            &sphereMesh_,
            XMMatrixScaling(0.55f, 0.55f, 0.55f) *
                XMMatrixTranslation(std::cosf(timeSeconds * 0.9f) * 2.4f, -0.55f + std::sinf(timeSeconds * 1.7f) * 0.25f, std::sinf(timeSeconds * 0.9f) * 2.4f),
            { XMFLOAT3(0.95f, 0.67f, 0.18f), 1.0f, 0.18f },
            true
        });

        scene.push_back({
            &sphereMesh_,
            XMMatrixScaling(0.45f, 0.45f, 0.45f) *
                XMMatrixTranslation(std::cosf(timeSeconds * 1.35f + 1.8f) * 3.1f, -0.65f, std::sinf(timeSeconds * 1.35f + 1.8f) * 1.6f),
            { XMFLOAT3(0.18f, 0.49f, 0.92f), 0.0f, 0.12f },
            true
        });

        scene.push_back({
            &cubeMesh_,
            XMMatrixScaling(0.42f, 0.42f, 0.42f) *
                XMMatrixRotationRollPitchYaw(timeSeconds * 1.9f, -timeSeconds * 0.8f, timeSeconds * 0.55f) *
                XMMatrixTranslation(-2.2f, -0.55f + std::sinf(timeSeconds * 2.2f) * 0.45f, -1.7f),
            { XMFLOAT3(0.36f, 0.82f, 0.48f), 0.0f, 0.78f },
            true
        });

        scene.push_back({
            &cubeMesh_,
            XMMatrixScaling(0.35f, 0.9f, 0.35f) *
                XMMatrixRotationY(timeSeconds * 0.5f) *
                XMMatrixTranslation(2.6f, -0.35f, -1.4f),
            { XMFLOAT3(0.62f, 0.42f, 0.92f), 0.65f, 0.28f },
            true
        });

        return scene;
    }

    void UpdateObjectConstants(const RenderItem& item, const XMMATRIX& viewProjection, const XMMATRIX& lightViewProjection)
    {
        ObjectConstants constants = {};
        constants.world = XMMatrixTranspose(item.world);
        constants.worldViewProjection = XMMatrixTranspose(item.world * viewProjection);
        constants.lightWorldViewProjection = XMMatrixTranspose(item.world * lightViewProjection);
        constants.albedo = item.material.albedo;
        constants.metallic = item.material.metallic;
        constants.roughness = item.material.roughness;
        context_->UpdateSubresource(objectConstantBuffer_.Get(), 0, nullptr, &constants, 0, 0);
    }

    void DrawMesh(const Mesh& mesh)
    {
        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;
        ID3D11Buffer* vertexBuffer = mesh.vertexBuffer.Get();
        context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context_->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        context_->DrawIndexed(mesh.indexCount, 0, 0);
    }

    void RenderShadowPass(const std::vector<RenderItem>& scene, const XMMATRIX& lightViewProjection)
    {
        context_->OMSetRenderTargets(0, nullptr, shadowDepthView_.Get());
        context_->ClearDepthStencilView(shadowDepthView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        context_->RSSetViewports(1, &shadowViewport_);
        context_->RSSetState(shadowRasterizerState_.Get());
        context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);

        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(depthVertexShader_.Get(), nullptr, 0);
        context_->VSSetConstantBuffers(1, 1, objectConstantBuffer_.GetAddressOf());
        context_->PSSetShader(nullptr, nullptr, 0);

        for (const RenderItem& item : scene)
        {
            if (!item.castsShadow)
            {
                continue;
            }

            UpdateObjectConstants(item, XMMatrixIdentity(), lightViewProjection);
            DrawMesh(*item.mesh);
        }

        context_->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void RenderForwardPass(const std::vector<RenderItem>& scene, const XMMATRIX& viewProjection, const XMMATRIX& lightViewProjection)
    {
        const float clearColor[] = { 0.04f, 0.055f, 0.075f, 1.0f };
        context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
        context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
        context_->RSSetViewports(1, &viewport_);
        context_->RSSetState(rasterizerState_.Get());
        context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);

        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(forwardVertexShader_.Get(), nullptr, 0);
        context_->VSSetConstantBuffers(1, 1, objectConstantBuffer_.GetAddressOf());
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
        context_->PSSetConstantBuffers(0, 1, frameConstantBuffer_.GetAddressOf());
        context_->PSSetConstantBuffers(1, 1, objectConstantBuffer_.GetAddressOf());
        context_->PSSetShaderResources(0, 1, shadowResourceView_.GetAddressOf());
        context_->PSSetSamplers(0, 1, shadowSampler_.GetAddressOf());

        for (const RenderItem& item : scene)
        {
            UpdateObjectConstants(item, viewProjection, lightViewProjection);
            DrawMesh(*item.mesh);
        }

        ID3D11ShaderResourceView* nullResource = nullptr;
        context_->PSSetShaderResources(0, 1, &nullResource);
        ThrowIfFailed(swapChain_->Present(1, 0));
    }

    HWND hwnd_ = nullptr;
    UINT viewportWidth_ = 1;
    UINT viewportHeight_ = 1;
    D3D11_VIEWPORT viewport_ = {};
    D3D11_VIEWPORT shadowViewport_ = {};

    Mesh cubeMesh_;
    Mesh planeMesh_;
    Mesh sphereMesh_;

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain> swapChain_;
    ComPtr<ID3D11RenderTargetView> renderTargetView_;
    ComPtr<ID3D11Texture2D> depthStencilBuffer_;
    ComPtr<ID3D11DepthStencilView> depthStencilView_;
    ComPtr<ID3D11Texture2D> shadowMap_;
    ComPtr<ID3D11DepthStencilView> shadowDepthView_;
    ComPtr<ID3D11ShaderResourceView> shadowResourceView_;
    ComPtr<ID3D11DepthStencilState> depthStencilState_;
    ComPtr<ID3D11RasterizerState> rasterizerState_;
    ComPtr<ID3D11RasterizerState> shadowRasterizerState_;
    ComPtr<ID3D11SamplerState> shadowSampler_;
    ComPtr<ID3D11VertexShader> forwardVertexShader_;
    ComPtr<ID3D11VertexShader> depthVertexShader_;
    ComPtr<ID3D11PixelShader> pixelShader_;
    ComPtr<ID3D11InputLayout> inputLayout_;
    ComPtr<ID3D11Buffer> frameConstantBuffer_;
    ComPtr<ID3D11Buffer> objectConstantBuffer_;
};

ForwardRenderer gRenderer;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        gRenderer.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow)
{
    try
    {
        WNDCLASSEX windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = kWindowClassName;
        RegisterClassEx(&windowClass);

        RECT windowRect = { 0, 0, static_cast<LONG>(kInitialWidth), static_cast<LONG>(kInitialHeight) };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowEx(
            0,
            kWindowClassName,
            L"DS Forward Renderer - PBR Shadow Scene",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            instance,
            nullptr);

        if (!hwnd)
        {
            return 1;
        }

        gRenderer.Initialize(hwnd, kInitialWidth, kInitialHeight);
        ShowWindow(hwnd, commandShow);

        const auto startTime = std::chrono::steady_clock::now();
        MSG message = {};
        while (message.message != WM_QUIT)
        {
            if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const float seconds = std::chrono::duration<float>(now - startTime).count();
            gRenderer.Render(seconds);
        }

        return static_cast<int>(message.wParam);
    }
    catch (const std::exception&)
    {
        MessageBox(nullptr, L"Renderer initialization or rendering failed.", L"DS", MB_ICONERROR | MB_OK);
        return 1;
    }
}
