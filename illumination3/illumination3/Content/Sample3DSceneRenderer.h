//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "..\Helpers\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Helpers\StepTimer.h"

namespace DirectXGame1
{
    // This sample renderer instantiates a basic rendering pipeline.
    class Sample3DSceneRenderer
    {
    public:
        Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();
		void RenderScreen();
        void StartTracking();
        void TrackingUpdate(float positionX);
        void StopTracking();
        bool IsTracking() { return m_tracking; }


    private:
        void Rotate(float radians);

    private:
        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// resources for render-to-texture

		ID3D11Texture2D* canvas;
		ID3D11RenderTargetView* RTV_canvas;
		ID3D11ShaderResourceView* SRV_canvas;

		D3D11_TEXTURE2D_DESC textureDesc;
		D3D11_RENDER_TARGET_VIEW_DESC RTV_canvas_desc;
		D3D11_SHADER_RESOURCE_VIEW_DESC SRV_canvas_desc;
		ID3D11SamplerState* m_sampler_screen = NULL;
		ModelViewProjectionConstantBuffer    m_constantBufferData_screen;

        // Direct3D resources for cube geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer_world;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer_world;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer_screen;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer_screen;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader_world;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader_world;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader_screen;

		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_constantBuffer;
		
		

        // System resources for cube geometry.
		ModelViewProjectionConstantBuffer    m_constantBufferData_world;
		uint32    m_indexCount;

        // Variables used with the rendering loop.
        bool    m_loadingComplete;
        float   m_degreesPerSecond;
        bool    m_tracking;
    };
}

