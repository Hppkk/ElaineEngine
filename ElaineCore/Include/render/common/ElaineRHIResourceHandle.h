#pragma once
#include "render/common/ElaineRHIResourceRef.h"
#include <type_traits>
#include <utility>

namespace Elaine
{
    // One intrusive owner for the actual RHI object. Raw pointers are borrowed.
    template<class T> class RHIResourceHandle
    {
    public:
        RHIResourceHandle() noexcept = default;
        RHIResourceHandle(std::nullptr_t) noexcept {}
        RHIResourceHandle(T* Ptr) noexcept : mResource(Ptr) { AddRef(); }
        RHIResourceHandle(const RHIResourceHandle& Other) noexcept : mResource(Other.mResource) { AddRef(); }
        RHIResourceHandle(RHIResourceHandle&& Other) noexcept : mResource(std::exchange(Other.mResource, nullptr)) {}
        template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        RHIResourceHandle(const RHIResourceHandle<U>& Other) noexcept : mResource(Other.get()) { AddRef(); }
        template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        RHIResourceHandle(RHIResourceHandle<U>&& Other) noexcept : mResource(std::exchange(Other.mResource, nullptr)) {}
        ~RHIResourceHandle() { if (mResource) mResource->DecreaseRef(); }
        RHIResourceHandle& operator=(RHIResourceHandle Other) noexcept { swap(Other); return *this; }
        template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        RHIResourceHandle& operator=(const RHIResourceHandle<U>& Other) noexcept { RHIResourceHandle Copy(Other); swap(Copy); return *this; }
        template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        RHIResourceHandle& operator=(RHIResourceHandle<U>&& Other) noexcept { RHIResourceHandle Move(std::move(Other)); swap(Move); return *this; }
        void reset(T* Ptr = nullptr) noexcept { RHIResourceHandle(Ptr).swap(*this); }
        void swap(RHIResourceHandle& Other) noexcept { std::swap(mResource, Other.mResource); }
        T* get() const noexcept { return mResource; }
        T* operator->() const noexcept { return mResource; }
        T& operator*() const noexcept { return *mResource; }
        explicit operator bool() const noexcept { return mResource != nullptr; }
        bool operator==(std::nullptr_t) const noexcept { return !mResource; }
        bool operator!=(std::nullptr_t) const noexcept { return mResource != nullptr; }
    private:
        void AddRef() noexcept { if (mResource) mResource->IncreaseRef(); }
        T* mResource = nullptr;
        template<class U> friend class RHIResourceHandle;
    };
}
