/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include "pch.h"
#include "BottomLevelASD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "DXGITypeConversions.hpp"
#include "StringTools.hpp"

namespace Diligent
{

BottomLevelASD3D12Impl::BottomLevelASD3D12Impl(IReferenceCounters*          pRefCounters,
                                               class RenderDeviceD3D12Impl* pDeviceD3D12,
                                               const BottomLevelASDesc&     Desc,
                                               bool                         bIsDeviceInternal) :
    TBottomLevelASBase{pRefCounters, pDeviceD3D12, Desc, bIsDeviceInternal}
{
    auto*  pd3d12Device             = pDeviceD3D12->GetD3D12Device5();
    UINT64 ResultDataMaxSizeInBytes = 0;

    if (m_Desc.CompactedSize)
    {
        ResultDataMaxSizeInBytes = m_Desc.CompactedSize;
    }
    else
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3d12BottomLevelPrebuildInfo = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS  d3d12BottomLevelInputs       = {};
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>           d3d12Geometries;

        if (m_Desc.pTriangles != nullptr)
        {
            d3d12Geometries.resize(m_Desc.TriangleCount);
            Uint32 MaxPrimitiveCount = 0;
            for (uint32_t i = 0; i < m_Desc.TriangleCount; ++i)
            {
                auto& src = m_Desc.pTriangles[i];
                auto& dst = d3d12Geometries[i];

                dst.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                dst.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
                dst.Triangles.VertexBuffer.StartAddress  = 0;
                dst.Triangles.VertexBuffer.StrideInBytes = 0;

                // Vertex format must be one of the following (https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_raytracing_geometry_triangles_desc):
                //  * DXGI_FORMAT_R32G32_FLOAT       - third component is assumed 0
                //  * DXGI_FORMAT_R32G32B32_FLOAT
                //  * DXGI_FORMAT_R16G16_FLOAT       - third component is assumed 0
                //  * DXGI_FORMAT_R16G16B16A16_FLOAT - A16 component is ignored, other data can be packed there, such as setting vertex stride to 6 bytes.
                //  * DXGI_FORMAT_R16G16_SNORM       - third component is assumed 0
                //  * DXGI_FORMAT_R16G16B16A16_SNORM - A16 component is ignored, other data can be packed there, such as setting vertex stride to 6 bytes.
                // Note that DXGI_FORMAT_R16G16B16A16_FLOAT and DXGI_FORMAT_R16G16B16A16_SNORM are workarounds for missing 16-bit 3-component DXGI formats
                switch (src.VertexValueType)
                {
                    case VT_FLOAT16:
                        switch (src.VertexComponentCount)
                        {
                            case 2: dst.Triangles.VertexFormat = DXGI_FORMAT_R16G16_FLOAT; break;
                            case 3: dst.Triangles.VertexFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
                            default: UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                        }
                        break;

                    case VT_FLOAT32:
                        switch (src.VertexComponentCount)
                        {
                            case 2: dst.Triangles.VertexFormat = DXGI_FORMAT_R32G32_FLOAT; break;
                            case 3: dst.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; break;
                            default: UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                        }
                        break;

                    case VT_INT16:
                        switch (src.VertexComponentCount)
                        {
                            case 2: dst.Triangles.VertexFormat = DXGI_FORMAT_R16G16_SNORM; break;
                            case 3: dst.Triangles.VertexFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
                            default: UNEXPECTED("Only 2 and 3 component vertex formats are expected");
                        }
                        break;

                    default:
                        UNEXPECTED(GetValueTypeString(src.VertexValueType), " is not a valid vertex component type");
                }

                dst.Triangles.VertexCount  = src.MaxVertexCount;
                dst.Triangles.IndexCount   = src.IndexType == VT_UNDEFINED ? 0 : src.MaxPrimitiveCount * 3;
                dst.Triangles.IndexFormat  = ValueTypeToIndexType(src.IndexType);
                dst.Triangles.IndexBuffer  = 0;
                dst.Triangles.Transform3x4 = 0;

                MaxPrimitiveCount += src.MaxPrimitiveCount;
            }
            VERIFY_EXPR(MaxPrimitiveCount <= D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE);
        }
        else if (m_Desc.pBoxes != nullptr)
        {
            d3d12Geometries.resize(m_Desc.BoxCount);
            Uint32 MaxBoxCount = 0;
            for (uint32_t i = 0; i < m_Desc.BoxCount; ++i)
            {
                auto& src = m_Desc.pBoxes[i];
                auto& dst = d3d12Geometries[i];

                dst.Type                      = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
                dst.Flags                     = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
                dst.AABBs.AABBCount           = src.MaxBoxCount;
                dst.AABBs.AABBs.StartAddress  = 0;
                dst.AABBs.AABBs.StrideInBytes = 0;

                MaxBoxCount += src.MaxBoxCount;
            }
            VERIFY_EXPR(MaxBoxCount <= D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE);
        }
        else
        {
            UNEXPECTED("Either pTriangles or pBoxes must not be null");
        }

        VERIFY_EXPR(d3d12Geometries.size() <= D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE);

        d3d12BottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        d3d12BottomLevelInputs.Flags          = BuildASFlagsToD3D12ASBuildFlags(m_Desc.Flags);
        d3d12BottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
        d3d12BottomLevelInputs.pGeometryDescs = d3d12Geometries.data();
        d3d12BottomLevelInputs.NumDescs       = static_cast<UINT>(d3d12Geometries.size());

        pd3d12Device->GetRaytracingAccelerationStructurePrebuildInfo(&d3d12BottomLevelInputs, &d3d12BottomLevelPrebuildInfo);
        if (d3d12BottomLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
            LOG_ERROR_AND_THROW("Failed to get ray tracing acceleration structure prebuild info");

        ResultDataMaxSizeInBytes = d3d12BottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;

        m_ScratchSize.Build  = static_cast<Uint32>(d3d12BottomLevelPrebuildInfo.ScratchDataSizeInBytes);
        m_ScratchSize.Update = static_cast<Uint32>(d3d12BottomLevelPrebuildInfo.UpdateScratchDataSizeInBytes);
    }

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ASDesc = {};
    ASDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    ASDesc.Alignment           = 0;
    ASDesc.Width               = ResultDataMaxSizeInBytes;
    ASDesc.Height              = 1;
    ASDesc.DepthOrArraySize    = 1;
    ASDesc.MipLevels           = 1;
    ASDesc.Format              = DXGI_FORMAT_UNKNOWN;
    ASDesc.SampleDesc.Count    = 1;
    ASDesc.SampleDesc.Quality  = 0;
    ASDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ASDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                    &ASDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                    __uuidof(m_pd3d12Resource),
                                                    reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
    if (FAILED(hr))
        LOG_ERROR_AND_THROW("Failed to create D3D12 Bottom-level acceleration structure");

    if (*m_Desc.Name != 0)
        m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

    VERIFY_EXPR(GetGPUAddress() % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0);

    SetState(RESOURCE_STATE_BUILD_AS_READ);
}

BottomLevelASD3D12Impl::~BottomLevelASD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    auto* pDeviceD3D12Impl = ValidatedCast<RenderDeviceD3D12Impl>(GetDevice());
    pDeviceD3D12Impl->SafeReleaseDeviceObject(std::move(m_pd3d12Resource), m_Desc.CommandQueueMask);
}

} // namespace Diligent
