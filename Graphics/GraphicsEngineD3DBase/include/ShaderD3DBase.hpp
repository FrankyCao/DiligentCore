/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#pragma once

#include "WinHPreface.h"
#include <d3dcommon.h>
#include "WinHPostface.h"

#include <functional>

#include "Shader.h"
#include "ThreadPool.h"
#include "RefCntAutoPtr.hpp"

/// \file
/// Base implementation of a D3D shader

namespace Diligent
{

class IDXCompiler;

/// Base implementation of a D3D shader
class ShaderD3DBase
{
public:
    void GetBytecode(const void** ppBytecode,
                     Uint64&      Size) const
    {
        DEV_CHECK_ERR(!m_pCompileTask, "Shader bytecode is not available until compilation is complete. Use GetStatus() to check the shader status.");
        if (m_pShaderByteCode)
        {
            *ppBytecode = m_pShaderByteCode->GetBufferPointer();
            Size        = m_pShaderByteCode->GetBufferSize();
        }
        else
        {
            *ppBytecode = nullptr;
            Size        = 0;
        }
    }

    SHADER_STATUS GetStatus(bool WaitForCompletion)
    {
        if (m_pCompileTask)
        {
            if (WaitForCompletion)
                m_pCompileTask->WaitForCompletion();

            if (m_pCompileTask->GetStatus() <= ASYNC_TASK_STATUS_RUNNING)
                return SHADER_STATUS_COMPILING;
            else
                m_pCompileTask.Release();
        }

        return m_pShaderByteCode ? SHADER_STATUS_READY : SHADER_STATUS_FAILED;
    }

    ID3DBlob* GetD3DBytecode() const
    {
        DEV_CHECK_ERR(!m_pCompileTask, "Shader bytecode is not available until compilation is complete. Use GetStatus() to check the shader status.");
        return m_pShaderByteCode;
    }

protected:
    void Initialize(const ShaderCreateInfo& ShaderCI,
                    const ShaderVersion     ShaderModel,
                    IDXCompiler*            DxCompiler,
                    IDataBlob**             ppCompilerOutput,
                    IThreadPool*            pAsyncCompilationThreadPool,
                    std::function<void()>   InitResources);

private:
    void InitializeInternal(const ShaderCreateInfo& ShaderCI,
                            const ShaderVersion     ShaderModel,
                            IDXCompiler*            DxCompiler,
                            IDataBlob**             ppCompilerOutput,
                            std::function<void()>   InitResources) noexcept(false);

protected:
    CComPtr<ID3DBlob>         m_pShaderByteCode;
    RefCntAutoPtr<IAsyncTask> m_pCompileTask;
};

} // namespace Diligent
