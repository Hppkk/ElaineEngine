#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderSystem.h"

namespace Elaine
{
    class RenderContext;
    using RenderCommand = std::function<void(RenderContext&)>;

    class ElaineCoreExport RenderCommandQueue
    {
    public:
        void Enqueue(RenderCommand&& Cmd)
        {
            mCommands[mWriteBuffer.load(std::memory_order_acquire)].push_back(std::move(Cmd));
        }

        void Execute()
        {
            int readBuffer = mWriteBuffer.load(std::memory_order_acquire);
            int writeBuffer = 1 - readBuffer;
            mWriteBuffer.store(writeBuffer, std::memory_order_release);

            auto& commands = mCommands[readBuffer];
            for (auto& Cmd : commands)
            {
                Cmd(RenderSystem::instance()->GetRenderContext());
            }

            commands.clear();
        }

        void Swap()
        {

        }

    private:
        std::atomic<int> mWriteBuffer{ 0 };
        std::atomic<bool> mCanSwap{ false };
        std::array<std::vector<RenderCommand>, 2> mCommands;
    };

#ifndef ENQUEUE_RENDER_COMMAND
#define ENQUEUE_RENDER_COMMAND(Name) \
    Elaine::RenderSystem::instance()->GetRenderCommandQueue()->Enqueue
#endif
}