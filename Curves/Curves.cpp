#include "DXUT.h"
#include "Vertex.h"
#include <cmath>

class Curves : public App
{
private:
    ID3D12RootSignature* rootSignature;
    ID3D12PipelineState* pipelineState;

    Mesh* controlVertexGeometry;
    Mesh* curveGeometry;

    static const uint CONTROL_VERTEX_QUANTITY = 80;
    Vertex controlVertexes[CONTROL_VERTEX_QUANTITY];

    static const uint CURVE_VERTEX_QUANTITY = 28;
    Vertex curveVertexes[CURVE_VERTEX_QUANTITY];

    uint controlVertexCount = 0;
    uint controlVertexIndex = 0;

public:
    void Init();
    void Update();
    void Display();
    void Finalize();

    Vertex generateBezierPoint(XMFLOAT3* p1, XMFLOAT3* p2, XMFLOAT3* p3, XMFLOAT3* p4, float  t);
    float calculateBezierCoordinate(float p1, float p2, float p3, float p4, float t);

    void BuildRootSignature();
    void BuildPipelineState();
};

void Curves::Init()
{
    graphics->ResetCommands();

    const uint CONTROL_VERTEX_BUFFER_SIZE = CONTROL_VERTEX_QUANTITY * sizeof(Vertex);
    const uint CURVE_VERTEX_BUFFER_SIZE = CURVE_VERTEX_QUANTITY * sizeof(Vertex);

    controlVertexGeometry = new Mesh(CONTROL_VERTEX_BUFFER_SIZE, sizeof(Vertex));
    curveGeometry = new Mesh(CURVE_VERTEX_BUFFER_SIZE, sizeof(Vertex));

    BuildRootSignature();
    BuildPipelineState();        

    graphics->SubmitCommands();
}

void Curves::Update()
{
    float cx = float(window->CenterX());
    float cy = float(window->CenterY());
    float mx = float(input->MouseX());
    float my = float(input->MouseY());

    float x = (mx - cx) / cx;
    float y = (cy - my) / cy;

    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    controlVertexes[controlVertexIndex] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Blue) };

    if (input->KeyPress(VK_LBUTTON))
    {
        controlVertexIndex = (controlVertexIndex + 1) % CONTROL_VERTEX_QUANTITY;
        
        if (controlVertexCount < CONTROL_VERTEX_QUANTITY)
            controlVertexCount++;
    }

    if (controlVertexCount > 1) {
        curveVertexes[0] = controlVertexes[0];

        double tIncrement = 1.0 / (CURVE_VERTEX_QUANTITY - 1);

        for (uint i = 1; i < CURVE_VERTEX_QUANTITY - 1; i++) {
            curveVertexes[i] = generateBezierPoint(&controlVertexes[0].Pos, &controlVertexes[1].Pos, &controlVertexes[2].Pos, &controlVertexes[3].Pos, i * tIncrement);
        }

        curveVertexes[CURVE_VERTEX_QUANTITY - 1] = controlVertexes[CONTROL_VERTEX_QUANTITY - 1];
    }

    graphics->ResetCommands();
    graphics->Copy(controlVertexes, controlVertexGeometry->vertexBufferSize, controlVertexGeometry->vertexBufferUpload, controlVertexGeometry->vertexBufferGPU);
    graphics->Copy(curveVertexes, curveGeometry->vertexBufferSize, curveGeometry->vertexBufferUpload, curveGeometry->vertexBufferGPU);
    graphics->SubmitCommands();

    Display();
}

void Curves::Display()
{
    graphics->Clear(pipelineState);

    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

    graphics->CommandList()->IASetVertexBuffers(0, 1, controlVertexGeometry->VertexBufferView());
    graphics->CommandList()->DrawInstanced(controlVertexCount, 1, 0, 0);

    graphics->CommandList()->IASetVertexBuffers(0, 1, curveGeometry->VertexBufferView());
    graphics->CommandList()->DrawInstanced(CURVE_VERTEX_QUANTITY - 1, 1, 0, 0);

    graphics->Present();    
}

void Curves::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();

    delete controlVertexGeometry;
    delete curveGeometry;
}


Vertex Curves::generateBezierPoint(XMFLOAT3* p1, XMFLOAT3* p2, XMFLOAT3* p3, XMFLOAT3* p4, float  t) {
    float x = calculateBezierCoordinate(p1->x, p2->x, p3->x, p4->x, t);
    float y = calculateBezierCoordinate(p1->y, p2->y, p3->y, p4->y, t);

    Vertex newBezierVertex = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Aqua) };

    return newBezierVertex;
}

float Curves::calculateBezierCoordinate(float p1, float p2, float p3, float p4, float t) {
    return pow((1 - t), 3) * p1 + 3 * t * pow((1 - t), 2) * p2 + 3 * pow(t, 2) * (1 - t) * p3 + pow(t, 3) * p4;
}

void Curves::BuildRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

void Curves::BuildPipelineState()
{   
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    pixelShader->Release();
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance,    _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    try
    {
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 600);
        engine->window->ResizeMode(ASPECTRATIO);
        engine->window->Color(49, 47, 47);
        engine->window->Title("Curves");
        engine->window->Icon(IDI_ICON);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        engine->Start(new Curves());

        delete engine;
    }
    catch (Error & e)
    {
        MessageBox(nullptr, e.ToString().data(), "Curves", MB_OK);
    }

    return 0;
}