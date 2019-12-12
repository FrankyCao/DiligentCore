/*     Copyright 2019 Diligent Graphics LLC
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <thread>
#include <atomic>
#include <algorithm>

#include "TestingEnvironment.h"
#include "ThreadSignal.h"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

static const char g_ShaderSource[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
	pos = float4(0.0, 0.0, 0.0, 0.0);
}
                                       
void PSMain(out float4 col : SV_TARGET)
{
	col = float4(0.0, 0.0, 0.0, 0.0);
}
)";


class MultithreadedResourceCreationTest : public ::testing::Test
{
protected:
    ~MultithreadedResourceCreationTest();

    static void WorkerThreadFunc(MultithreadedResourceCreationTest* This, size_t ThreadNum);

    void StartWorkerThreadsAndWait(int SignalIdx);
    void WaitSiblingWorkerThreads(int SignalIdx);

    std::vector<std::thread> m_Threads;

    ThreadingTools::Signal m_WorkerThreadSignal[2];
    ThreadingTools::Signal m_MainThreadSignal;

    std::mutex      m_NumThreadsCompletedMtx;
    std::atomic_int m_NumThreadsCompleted[2];
    std::atomic_int m_NumThreadsReady;

    static const int NumBuffersToCreate  = 10;
    static const int NumTexturesToCreate = 5;
    static const int NumPSOToCreate      = 2;

#ifdef _DEBUG
    static const int NumIterations = 10;
#else
    static const int NumIterations = 100;
#endif
};

MultithreadedResourceCreationTest::~MultithreadedResourceCreationTest()
{
    LOG_INFO_MESSAGE("Created ", NumBuffersToCreate, " buffers, ", NumTexturesToCreate, " textures, and ",
                     NumPSOToCreate, " PSO in ", NumIterations, " iterations by each of ", m_Threads.size(), " threads");
}

void MultithreadedResourceCreationTest::WaitSiblingWorkerThreads(int SignalIdx)
{
    auto NumThreads = static_cast<int>(m_Threads.size());
    if (++m_NumThreadsCompleted[SignalIdx] == NumThreads)
    {
        ASSERT_FALSE(m_WorkerThreadSignal[1 - SignalIdx].IsTriggered());
        m_MainThreadSignal.Trigger();
    }
    else
    {
        while (m_NumThreadsCompleted[SignalIdx] < NumThreads)
            std::this_thread::yield();
    }
}

void MultithreadedResourceCreationTest::StartWorkerThreadsAndWait(int SignalIdx)
{
    m_NumThreadsCompleted[SignalIdx] = 0;
    m_WorkerThreadSignal[SignalIdx].Trigger(true);

    m_MainThreadSignal.Wait(true, 1);
}

void MultithreadedResourceCreationTest::WorkerThreadFunc(MultithreadedResourceCreationTest* This, size_t ThreadNum)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const int NumThreads = static_cast<int>(This->m_Threads.size());

    std::vector<Uint8> RawBufferData(1024);
    std::vector<Uint8> RawTextureData(1024 * 1024 * 4);

    int SignalIdx = 0;
    while(true)
    {
        auto SignaledValue = This->m_WorkerThreadSignal[SignalIdx].Wait(true, NumThreads);
        if (SignaledValue < 0)
        {
            return;
        }

        for (Uint32 i = 0; i < NumBuffersToCreate; ++i)
        {
            RefCntAutoPtr<IBuffer>     pBuffer1, pBuffer2, pBuffer3, pBuffer4;
            RefCntAutoPtr<IBufferView> pBufferSRV, pBufferUAV;

            BufferDesc BuffDesc;
            BuffDesc.Usage         = USAGE_DEFAULT;
            BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;
            BuffDesc.Name          = "MT creation test buffer";
            BuffDesc.uiSizeInBytes = static_cast<Uint32>(RawBufferData.size());

            BufferData BuffData;
            BuffData.DataSize = BuffDesc.uiSizeInBytes;
            BuffData.pData    = RawBufferData.data();

            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer1);

            BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
            BuffDesc.ElementByteStride = 16;
            BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer2);

            BufferViewDesc ViewDesc;
            ViewDesc.ViewType             = BUFFER_VIEW_SHADER_RESOURCE;
            ViewDesc.ByteOffset           = 16;
            ViewDesc.Format.NumComponents = 4;
            ViewDesc.Format.IsNormalized  = False;
            ViewDesc.Format.ValueType     = VT_FLOAT32;
            pBuffer2->CreateView(ViewDesc, &pBufferSRV);

            BuffDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer3);
            ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
            pBuffer3->CreateView(ViewDesc, &pBufferUAV);

            BuffDesc.Mode      = BUFFER_MODE_RAW;
            BuffDesc.BindFlags = BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer4);
        }

        for (Uint32 i = 0; i < NumTexturesToCreate; ++i)
        {
            RefCntAutoPtr<ITexture> pTexture;
            {
                TextureDesc TexDesc;
                TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
                TexDesc.Type      = RESOURCE_DIM_TEX_2D;
                TexDesc.Width     = 1024;
                TexDesc.Height    = 1024;
                TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
                TexDesc.MipLevels = 1;


                TextureSubResData SubResData;
                SubResData.pData  = RawTextureData.data();
                SubResData.Stride = TexDesc.Width * 4;

                TextureData TexData;
                TexData.NumSubresources = 1;
                TexData.pSubResources   = &SubResData;

                pDevice->CreateTexture(TexDesc, &TexData, &pTexture);
            }
        }

        for (Uint32 i = 0; i < NumPSOToCreate; ++i)
        {
            RefCntAutoPtr<IShader>        pTrivialVS, pTrivialPS;
            RefCntAutoPtr<IPipelineState> pPSO;
            {
                ShaderCreateInfo Attrs;
                Attrs.Source                     = g_ShaderSource;
                Attrs.EntryPoint                 = "VSMain";
                Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
                Attrs.Desc.Name                  = "TrivialVS (MTResourceCreationTest)";
                Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
                Attrs.UseCombinedTextureSamplers = true;
                pDevice->CreateShader(Attrs, &pTrivialVS);

                Attrs.EntryPoint      = "PSMain";
                Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
                Attrs.Desc.Name       = "TrivialPS (MTResourceCreationTest)";
                pDevice->CreateShader(Attrs, &pTrivialPS);

                PipelineStateDesc PSODesc;
                PSODesc.GraphicsPipeline.pVS               = pTrivialVS;
                PSODesc.GraphicsPipeline.pPS               = pTrivialPS;
                PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                PSODesc.GraphicsPipeline.NumRenderTargets  = 1;
                PSODesc.GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
                PSODesc.GraphicsPipeline.DSVFormat         = TEX_FORMAT_D32_FLOAT;

                pDevice->CreatePipelineState(PSODesc, &pPSO);
            }
        }

        This->WaitSiblingWorkerThreads(SignalIdx);
        SignalIdx = 1 - SignalIdx;
    }
}


TEST_F(MultithreadedResourceCreationTest, CreateResources)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceCaps().IsGLDevice())
    {
        GTEST_SKIP() << "Multithreading resource creation is not supported in OpenGL";
    }

    TestingEnvironment::ScopedReleaseResources AutoResetEnvironment;

    auto numCores = std::thread::hardware_concurrency();
    m_Threads.resize(std::max(numCores, 4u));
    for (auto& t : m_Threads)
        t = std::thread(WorkerThreadFunc, this, &t - m_Threads.data());

    int SignalIdx = 0;
    for (Uint32 iter = 0; iter < NumIterations; ++iter)
    {
        StartWorkerThreadsAndWait(SignalIdx);
        pEnv->ReleaseResources();
        SignalIdx = 1 - SignalIdx;
    }

    m_WorkerThreadSignal[SignalIdx].Trigger(true, -1);
    for (auto& t : m_Threads)
        t.join();
}

} // namespace
