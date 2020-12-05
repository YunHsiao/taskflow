#include "CommandBuffer.h"
#include <cassert>

namespace
{
uint32_t constexpr kMemoryChunkSize = 4096 * 16;
uint32_t constexpr kMemoryChunkPoolCapacity = 64;
uint32_t constexpr kSwitchChunkMemoryRequirement = sizeof(MemoryChunkSwitchCommand) + sizeof(DummyCommand);
}

CommandBuffer::MemoryAllocator& CommandBuffer::MemoryAllocator::GetInstance() noexcept
{
    static CommandBuffer::MemoryAllocator instance;
    return instance;
}

uint8_t* CommandBuffer::MemoryAllocator::Request() noexcept
{
    uint8_t* newChunk = nullptr;

    if (mChunkPool.try_dequeue(newChunk))
    {
        mChunkCount.fetch_sub(1, std::memory_order_acq_rel);
    }
    else
    {
        newChunk = MemoryAllocateForMultiThread<uint8_t>(kMemoryChunkSize);
    }

    return newChunk;
}

void CommandBuffer::MemoryAllocator::Recycle(uint8_t* const chunk, bool const freeByUser) noexcept
{
    if (freeByUser)
    {
        mChunkFreeQueue.enqueue(chunk);
    }
    else
    {
        Free(chunk);
    }
}

void CommandBuffer::MemoryAllocator::FreeByUser(CommandBuffer* const mainCommandbuffer) noexcept
{
    auto queue = &mChunkFreeQueue;

    ENCODE_COMMAND_1(
            mainCommandbuffer, FreeChunksInFreeQueue,
            queue, queue,
            {
                uint8_t* chunk = nullptr;

                while(queue->try_dequeue(chunk))
                {
                    CommandBuffer::MemoryAllocator::GetInstance().Free(chunk);
                }
            });

    mainCommandbuffer->Kick();
}

void CommandBuffer::MemoryAllocator::Free(uint8_t* const chunk) noexcept
{
    if (mChunkCount.load(std::memory_order_acquire) >= kMemoryChunkPoolCapacity)
    {
        MemoryFreeForMultiThread(chunk);
    }
    else
    {
        mChunkPool.enqueue(chunk);
        mChunkCount.fetch_add(1, std::memory_order_acq_rel);
    }
}

CommandBuffer::CommandBuffer()
{
    uint8_t* const chunk = MemoryAllocator::GetInstance().Request();

    mW.mCurrentMemoryChunk = chunk;
    mR.mCurrentMemoryChunk = chunk;

    // 分配一个头节点 不要被Execute
    Command* const cmd = Allocate<DummyCommand>(1);
    PushCommands();
    PullCommands();
    mR.mLastCommand = cmd;
    --mR.mNewCommandCount;
}

void CommandBuffer::Kick() noexcept
{
    PushCommands();
    mN.Signal();
}

void CommandBuffer::KickAndWait() noexcept
{
    EventSem event;
    EventSem* const pEvent = &event;

    ENCODE_COMMAND_1(this, WaitUntilFinish,
            pEvent, pEvent,
            {
                pEvent->Signal();
            });

    Kick();
    event.Wait();
}

void CommandBuffer::RunConsumerThread() noexcept
{
    if (mWorkerAttached)
    {
        return;
    }

    std::thread consumerThread(std::bind(&CommandBuffer::ConsumerThreadLoop, this));
    consumerThread.detach();
    mWorkerAttached = true;
}

void CommandBuffer::TerminateConsumerThread() noexcept
{
    if (! mWorkerAttached)
    {
        return;
    }

    EventSem event;
    EventSem* const pEvent = &event;

    ReaderContext* const pR = &mR;
    ENCODE_COMMAND_2(
            this,   TerminateConsumerThread,
             pR,    pR,
            pEvent, pEvent,
            {
                pR->mTerminateConsumerThread = true;
                pR->mFlushingFinished = true;
                pEvent->Signal();
            });

    Kick();
    event.Wait();
}

void CommandBuffer::FinishWriting() noexcept
{
    if (!mImmediateMode)
    {
        bool* const flushingFinished = &mR.mFlushingFinished;

        ENCODE_COMMAND_1(this, FinishWriting,
                flushingFinished, flushingFinished,
                {
                    *flushingFinished = true;
                });

        Kick();
    }
}

void CommandBuffer::RecycleMemoryChunk(uint8_t* const chunk) const noexcept
{
    CommandBuffer::MemoryAllocator::GetInstance().Recycle(chunk, mFreeChunksByUser);
}

void CommandBuffer::FreeChunksInFreeQueue(CommandBuffer* const mainCommandbuffer) noexcept
{
    CommandBuffer::MemoryAllocator::GetInstance().FreeByUser(mainCommandbuffer);
}

uint8_t* CommandBuffer::AllocateImpl(uint32_t& allocatedSize, uint32_t const requestSize) noexcept
{
    uint32_t const alignedSize = Align(requestSize, 16);
    assert(alignedSize + kSwitchChunkMemoryRequirement <= kMemoryChunkSize);    // 一个Command直接把整个Chunk填满了

    uint32_t const newOffset = mW.mOffset + alignedSize;

    if (newOffset + kSwitchChunkMemoryRequirement <= kMemoryChunkSize)
    {
        allocatedSize = alignedSize;
        uint8_t* const allocatedMemory = mW.mCurrentMemoryChunk + mW.mOffset;
        mW.mOffset = newOffset;
        return allocatedMemory;
    }
    else
    {
        uint8_t* const newChunk = CommandBuffer::MemoryAllocator::GetInstance().Request();
        MemoryChunkSwitchCommand* const switchCommand = reinterpret_cast<MemoryChunkSwitchCommand*>(mW.mCurrentMemoryChunk + mW.mOffset);
        new(switchCommand) MemoryChunkSwitchCommand(this, newChunk, mW.mCurrentMemoryChunk);
        switchCommand->mNext = reinterpret_cast<Command*>(newChunk);  // 注意这里还指向原位置
        mW.mLastCommand = switchCommand;
        ++mW.mPendingCommandCount;
        mW.mCurrentMemoryChunk = newChunk;
        mW.mOffset = 0;

        DummyCommand* const head = Allocate<DummyCommand>(1);
        new(head) DummyCommand;

        if (mImmediateMode)
        {
            PushCommands();
            PullCommands();
            assert(mR.mNewCommandCount == 2);
            ExecuteCommand();
            ExecuteCommand();
        }

        return AllocateImpl(allocatedSize, requestSize);
    }
}

void CommandBuffer::PushCommands() noexcept
{
    mW.mWrittenCommandCount.fetch_add(mW.mPendingCommandCount, std::memory_order_acq_rel);
    mW.mPendingCommandCount = 0;
}

void CommandBuffer::PullCommands() noexcept
{
    uint32_t const writtenCommandCountNew = mW.mWrittenCommandCount.load(std::memory_order_acquire);
    mR.mNewCommandCount += writtenCommandCountNew - mR.mWrittenCommandCountSnap;
    mR.mWrittenCommandCountSnap = writtenCommandCountNew;
}

void CommandBuffer::FlushCommands() noexcept
{
    while (! mR.mFlushingFinished)
    {
        ExecuteCommand();
    }

    mR.mFlushingFinished = false;
}

void CommandBuffer::ExecuteCommand() noexcept
{
    Command* const cmd = ReadCommand();

    if (! cmd)
    {
        return;
    }

    cmd->Execute();
    cmd->~Command();
}

Command* CommandBuffer::ReadCommand() noexcept
{
    while (! HasNewCommand())
    {
        // 如果当前没有可读的Command 尝试同步一下生产者线程的数据
        PullCommands();

        // 如果依然没有 挂起消费者线程 等待生产者线程填充Command并唤醒消费者线程
        if (! HasNewCommand())
        {
            mN.Wait();
            PullCommands();
        }
    }

    Command* const cmd = mR.mLastCommand->GetNext();
    mR.mLastCommand = cmd;
    --mR.mNewCommandCount;
    return cmd;
}

void CommandBuffer::ConsumerThreadLoop() noexcept
{
    while (! mR.mTerminateConsumerThread)
    {
        FlushCommands();
    }

    mWorkerAttached = false;
}

char const* DummyCommand::GetName() const noexcept
{
    return "Dummy";
}

MemoryChunkSwitchCommand::MemoryChunkSwitchCommand(CommandBuffer* const cb, uint8_t* const newChunk, uint8_t* const oldChunk) noexcept
    : mCommandBuffer(cb)
    , mNewChunk(newChunk)
    , mOldChunk(oldChunk)
{
}

MemoryChunkSwitchCommand::~MemoryChunkSwitchCommand()
{
    mCommandBuffer->RecycleMemoryChunk(mOldChunk);
}

void MemoryChunkSwitchCommand::Execute() noexcept
{
    mCommandBuffer->mR.mCurrentMemoryChunk = mNewChunk;
    mCommandBuffer->PullCommands();
}

char const* MemoryChunkSwitchCommand::GetName() const noexcept
{
    return "MemoryChunkSwitch";
}
