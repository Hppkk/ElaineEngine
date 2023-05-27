#pragma once

#include "render/common/ElaineRHITypes.h"
#include <d3d12.h>

namespace Elaine
{
    class DX12Buffer : public RHIBuffer
    {
    public:
        void setResource(ID3D12Resource* res)
        {
            m_resource = res;
        }
        ID3D12Resource* getResource() const
        {
            return m_resource;
        }
    private:
        ID3D12Resource* m_resource;
    };

    class DX12BufferView : public RHIBufferView
    {
    public:
        void setResource(D3D12_BUFFER_RTV res)
        {
            m_resource = res;
        }
        D3D12_BUFFER_RTV getResource() const
        {
            return m_resource;
        }
    private:
        D3D12_BUFFER_RTV m_resource;
    };

}