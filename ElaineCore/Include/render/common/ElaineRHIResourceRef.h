#pragma once
#include <atomic>
#include <cstdint>

namespace Elaine
{
    template<class T> class RHIResourceHandle;

    template<class Ty> class RHIResourceRef
    {
    public:
        uint32_t UseCount() const noexcept { return mRefCount.load(std::memory_order_acquire); }
    protected:
        RHIResourceRef() = default;
        ~RHIResourceRef() = default;
    private:
        void IncreaseRef() noexcept { mRefCount.fetch_add(1, std::memory_order_relaxed); }
        void DecreaseRef() noexcept
        {
            if (mRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                static_cast<Ty*>(this)->OnZeroReferences();
        }
        std::atomic_uint32_t mRefCount{0};
        template<class T> friend class RHIResourceHandle;
    };
}
