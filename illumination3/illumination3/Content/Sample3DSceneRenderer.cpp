//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Helpers\DirectXHelper.h"

using namespace DirectXGame1;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_loadingComplete(false),
    m_degreesPerSecond(45),
    m_indexCount(0),
    m_tracking(false),
    m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.1f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData_world.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.0f, 1.5f, 1.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	static XMVECTORF32 light = { 2.0f, 2.0f, 2.0f, 1.0f };
	FXMVECTOR eyee = { 0.7f, 0.7f, 1.5f, 1.0f };
	FXMVECTOR lighte = { 2.0f, 2.0f, 2.0f, 1.0f };

	XMStoreFloat4x4(&m_constantBufferData_world.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	XMStoreFloat4(&m_constantBufferData_world.eyepos, eye);
	XMStoreFloat4(&m_constantBufferData_world.lightpos, light);
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
    if (!m_tracking)
    {
        // Convert degrees to radians, then convert seconds to rotation angle
        float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
        double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
        float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

        Rotate(radians);
    }
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
    // Prepare to pass the updated model matrix to the shader
	static int pk = 0;
	pk++;
	float t = 5*sin(pk / 25.0f);
	FXMVECTOR eye = { 0.0, 0.6, 1.0, 1.0 };
	FXMVECTOR light = { t, 2, 2, 1 }; // moves back and forth in x

	XMStoreFloat4x4(&m_constantBufferData_world.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
	//XMStoreFloat4(&m_constantBufferData.eyepos, eye); // eye didn't move, don't worry about it
	XMStoreFloat4(&m_constantBufferData_world.lightpos, light);
	
}

void Sample3DSceneRenderer::StartTracking()
{
    m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
    if (m_tracking)
    {
        float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
        Rotate(radians);
    }
}

void Sample3DSceneRenderer::StopTracking()
{
    m_tracking = false;
}
/*--------------------------------------------------------------------------------------------------------------------*/
// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { RTV_canvas };
//	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };

	static int pk = 0;
	pk++;

	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());
	
	context->ClearRenderTargetView(RTV_canvas, DirectX::Colors::Black);
		
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData_world,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer_world.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer_world.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader_world.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader_world.Get(),
		nullptr,
		0
		);

	context->PSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}
/*----------------------------------------------------------------------------------------------------------*/
void Sample3DSceneRenderer::RenderScreen()
{
	// copied from ::Render
	// the plan: set render target to screen
	// then, render quad geometry
	// note, constant buffer should contain ortho projection
	static int pk = 0;
	pk++;
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };

	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::Cornsilk);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMStoreFloat4x4(&m_constantBufferData_screen.model, XMMatrixIdentity());
//	XMStoreFloat4x4(&m_constantBufferData_screen.model, XMMatrixTranspose(XMMatrixRotationY(pk / 200.0f)));
	static const XMVECTORF32 eye = { 0.0f, 0.0f, -100.5f, 1.0f };
	static const XMVECTORF32 gaze = { 0.0f, 0.0f, 1.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMVECTORF32 timer = { pk, 0.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&m_constantBufferData_screen.view, XMMatrixTranspose(XMMatrixLookToRH(eye, gaze, up)));
	XMStoreFloat4(&m_constantBufferData_screen.lightpos, timer);
	XMStoreFloat4x4(&m_constantBufferData_screen.projection, XMMatrixTranspose(XMMatrixOrthographicRH(m_deviceResources->GetOutputSize().Width, m_deviceResources->GetOutputSize().Height, 1, 500)));
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData_screen,
		0,
		0
		);
	
	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer_screen.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer_screen.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader_world.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader_screen.Get(),
		nullptr,
		0
		);

	context->PSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);
	
	// set sampler and texture for pixel shader

	context->PSSetShaderResources(0, 1, &SRV_canvas);
	context->PSSetSamplers(0, 1, &m_sampler_screen);


	// Draw the objects, i.e., the quad
	context->DrawIndexed(
		6,
		0,
		0
		);
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
    // Load shaders asynchronously.
    auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadPS2Task = DX::ReadDataAsync(L"screenps.cso");

    // After the vertex shader file is loaded, create the shader and input layout.
    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_vertexShader_world
                )
            );

        static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateInputLayout(
                vertexDesc,
                ARRAYSIZE(vertexDesc),
                &fileData[0],
                fileData.size(),
                &m_inputLayout
                )
            );
    });

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			&m_pixelShader_world
			)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			&m_constantBuffer
			)
			);
	});


	// After the pixel shader file is loaded, create the shader and constant buffer.
	
	auto createPS2Task = loadPS2Task.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			&m_pixelShader_screen
			)
			);

	});

    // Once both shaders are loaded, create the mesh.
    auto createCubeTask = (createPSTask && createVSTask && createPS2Task).then([this] () {

        // Load mesh vertices. Each vertex has a position and a color.
        static const VertexPositionColor cubeVertices[] = 
        {
            {XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
            {XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
            {XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
            {XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
            {XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
            {XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
            {XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
        };


        // Load mesh indices. Each trio of indices represents
        // a triangle to be rendered on the screen.
        // For example: 0,2,1 means that the vertices with indexes
        // 0, 2 and 1 from the vertex buffer compose the 
        // first triangle of this mesh.
        static const unsigned short cubeIndices [] =
        {
            0,2,1, // -x
            1,2,3,

            4,5,6, // +x
            5,7,6,

            0,1,5, // -y
            0,5,4,

            2,6,7, // +y
            2,7,3,

            0,4,6, // -z
            0,6,2,

            1,3,7, // +z
            1,7,5,
        };

		int circle = 30;
		float theta, phi;
		XMFLOAT3 ccen;
		float trad = 0.6;
		float crad = 0.2;
		int loop;
		int segment = 30;
		loop = 3 * segment;
		int numvertices = circle*loop;
		int numindices = circle*loop * 6;
		VertexPositionColor *vertices;
		VertexPositionColor thisone;
		XMFLOAT3 thisnor;
		vertices = (VertexPositionColor *)malloc(numvertices*sizeof(VertexPositionColor));

		// torus:
		for (int i = 0; i < loop; i++)
		{
			
			theta = 2*i*3.1416/loop; // large loop
			//crad = 0.3 + 0.08*sin(7*theta); // vary small circle radius, 7 lobes
			ccen = XMFLOAT3(trad*cos(theta),trad*sin(theta),0); // centre of this small circle

			for (int j = 0; j < circle; j++) // small circle
			{
				
				phi = 2*j*3.1416 / circle; // from 0 to 2PI
				
				thisnor = // normal direction
					XMFLOAT3(cos(theta)*sin(phi), sin(theta)*sin(phi), cos(phi));
				thisone = // position + color of this vertex
				{
					XMFLOAT3(ccen.x+thisnor.x*crad, ccen.y+thisnor.y*crad, ccen.z+thisnor.z*crad),
					XMFLOAT3(i / (segment + 0.01), j / (circle + 0.01), 0.05),
					XMFLOAT3(thisnor.x, thisnor.y, thisnor.z)
				};
				vertices[i*circle + j] = thisone; // add to vertex array
			}
		}

		WORD *indices;
		indices = (WORD *)malloc(sizeof(WORD)*numindices);
		int count = 0;
		for (int i = 0; i < loop; i++)
		{
			for (int j = 0; j < circle; j++)
			{
				// two triangles per quad

				// proper order:

				indices[count++] = WORD(((i + 1) % loop)*circle + j);
				indices[count++] = WORD(i*circle + ((j + 1) % circle));
				indices[count++] = WORD((i*circle + j));
				
				// reversed:
				
				
				//indices[count++] = WORD((i*circle + j));	
				//indices[count++] = WORD(i*circle + ((j + 1) % circle));
			
				//indices[count++] = WORD(((i + 1) % loop)*circle + j);
				
				indices[count++] = WORD(((i + 1) % loop)*circle + j);
				indices[count++] = WORD(((i + 1) % loop)*circle + ((j + 1) % circle));
				indices[count++] = WORD(i*circle + ((j + 1) % circle));
			}
		}


		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = vertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(numvertices*sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer_world
			)
			);

		m_indexCount = numindices; // ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(numindices*sizeof(WORD), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_indexBuffer_world
			)
			);
    
		VertexPositionColor* fvertices;
		fvertices = (VertexPositionColor *)malloc(4 * sizeof(VertexPositionColor));
		int width = m_deviceResources->GetOutputSize().Width;
		int height = m_deviceResources->GetOutputSize().Height;
		float SZx = width / 2 ;
		float SZy = height / 2;

		fvertices[0].pos = XMFLOAT3(-SZx, -SZy, 0);
		fvertices[0].tex = XMFLOAT2(1, 1);

		fvertices[2].pos = XMFLOAT3(SZx, -SZy, 0);
		fvertices[2].tex = XMFLOAT2(0, 1);

		fvertices[1].pos = XMFLOAT3(-SZx, SZy, 0);
		fvertices[1].tex = XMFLOAT2(1, 0);

		fvertices[3].pos = XMFLOAT3(SZx, SZy, 0);
		fvertices[3].tex = XMFLOAT2(0, 0);
		
		WORD *findices;
		findices = (WORD *)malloc(sizeof(WORD)*6);
		findices[0] = 3;
		findices[1] = 1;
		findices[2] = 0;
		findices[3] = 2;
		findices[4] = 3;
		findices[5] = 0;

//		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = fvertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC fvertexBufferDesc(4*sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&fvertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer_screen
			)
			);

	//	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = findices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC findexBufferDesc(numindices*sizeof(WORD), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&findexBufferDesc,
			&indexBufferData,
			&m_indexBuffer_screen
			)
			);

		// adding creation of canvas here: texture itself, render target view, and shader resource view


		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width = m_deviceResources->GetOutputSize().Width;
		td.Height = m_deviceResources->GetOutputSize().Height;
		td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		td.SampleDesc.Count = 1;
		td.ArraySize = 1;
		td.MipLevels = 1;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // note, will be used as both RT and SR
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;

		m_deviceResources->GetD3DDevice()->CreateTexture2D(&td, NULL, &canvas);

		// render target view: render scene to canvas
		D3D11_RENDER_TARGET_VIEW_DESC rtvd;
		ZeroMemory(&rtvd, sizeof(rtvd));
		rtvd.Format = td.Format;
		rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvd.Texture2D.MipSlice = 0;
		m_deviceResources->GetD3DDevice()->CreateRenderTargetView(canvas, &rtvd, &RTV_canvas);

		// create shader resource view so we can connect ps to canvas
		SRV_canvas_desc.Format = td.Format;
		SRV_canvas_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRV_canvas_desc.Texture2D.MostDetailedMip = 0;
		SRV_canvas_desc.Texture2D.MipLevels = 1;
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(canvas, &SRV_canvas_desc, &SRV_canvas);

		// finally, make texture sampler here
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // shouldn't be used
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		m_deviceResources->GetD3DDevice()->CreateSamplerState(&sampDesc, &m_sampler_screen);

});

	
    // Once the cube is loaded, the object is ready to be rendered.
    createCubeTask.then([this] () {
        m_loadingComplete = true;
    });
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
    m_loadingComplete = false;
    m_vertexShader_world.Reset();
    m_inputLayout.Reset();
    m_pixelShader_world.Reset();
    m_constantBuffer.Reset();
    m_vertexBuffer_world.Reset();
    m_indexBuffer_world.Reset();
}