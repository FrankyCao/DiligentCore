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

#pragma once

/// \file
/// Definition of the Diligent::ISamplerD3D11 interface

#include "../../GraphicsEngine/interface/Sampler.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {31A3BFAF-738E-4D8C-AD18-B021C5D948DD}
static const struct INTERFACE_ID IID_SamplerD3D11 =
    {0x31a3bfaf, 0x738e, 0x4d8c, {0xad, 0x18, 0xb0, 0x21, 0xc5, 0xd9, 0x48, 0xdd}};

#if DILIGENT_CPP_INTERFACE

/// Exposes Direct3D11-specific functionality of a sampler object.
class ISamplerD3D11 : public ISampler
{
public:
    /// Returns a pointer to the ID3D11SamplerState interface of the internal Direct3D11 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    virtual ID3D11SamplerState* GetD3D11SamplerState() = 0;
};

#else

struct ISamplerD3D11Methods
{
    ID3D11SamplerState* (*GetD3D11Sampler)();
};

struct ISamplerD3D11Vtbl
{
    struct IObjectMethods       Object;
    struct IDeviceObjectMethods DeviceObject;
    //struct ISamplerMethods      Sampler;
    struct ISamplerD3D11Methods SamplerD3D11;
};

struct ISamplerD3D11
{
    struct ISamplerD3D11Vtbl* pVtbl;
};

#    define ISamplerD3D11_GetD3D11Sampler(This) (This)->pVtbl->SamplerD3D11.GetD3D11Sampler((struct ISamplerD3D11*)(This))

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
