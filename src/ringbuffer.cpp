#include "ringbuffer.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

// SPSC model
namespace znsreader
{
RingBuffer::RingBuffer(std::size_t max_size, bool use_huge_pages, WriterCallBack writerFn, ReaderCallBack readerFn)
    : m_use_huge_pages(use_huge_pages),
      m_max_size(max_size),
      m_write_index(0),
      m_read_index(0),
      m_reader(readerFn),
      m_writer(writerFn)
{
    const size_t rounded_size = align_size(max_size, m_use_huge_pages);

    if (m_use_huge_pages) {
        m_start = (unsigned char *)::mmap(nullptr, rounded_size, ZNS_MMAP_PERMS, ZNS_MMAP_FLAGS, -1, 0);
        if (m_start == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap");
        }
    } else {
        // TODO : posix_memalign?
        m_start = (unsigned char *)::malloc(rounded_size);
        if (m_start == nullptr) {
            throw std::runtime_error("Failed to malloc");
        }
    }

    m_end = (unsigned char *)m_start + m_max_size;
    m_allocated_size = rounded_size;

    std::cout << "Requested: " << max_size << ": Allocated: " << rounded_size << std::endl;
}

RingBuffer::~RingBuffer()
{
    if (m_use_huge_pages) {
        ::munmap((void *)m_start, m_allocated_size);
    } else {
        ::free(m_start);
    }
}

std::size_t RingBuffer::push(int fd, std::size_t max_bytes)
{
    const size_t write_index = m_write_index.load(std::memory_order_relaxed);
    const size_t read_index = m_read_index.load(std::memory_order_acquire);
    const size_t avail = write_available(write_index, read_index, m_max_size);

    if (avail < max_bytes) {
        return 0;
    }

    size_t new_write_index = write_index + max_bytes;

    if (new_write_index > m_max_size) {
        size_t written_bytes = m_writer(fd, m_start + write_index, 131072);

        if ((written_bytes + write_index) > m_max_size) {
            const size_t count0 = m_max_size - write_index;
            const size_t count1 = written_bytes - count0;

            ::memmove(m_start, (unsigned char *)m_end, count1);
            new_write_index = count1;
        } else {
            new_write_index = write_index + written_bytes;
        }
    } else {
        std::size_t written_bytes = m_writer(fd, m_start + write_index, max_bytes);

        if ((written_bytes + write_index) == m_max_size) {
            new_write_index = 0;
        } else {
            new_write_index = write_index + written_bytes;
        }
    }

    m_write_index.store(new_write_index, std::memory_order_release);

    return max_bytes;
}

std::size_t RingBuffer::pop_all()
{
    const size_t write_index = m_write_index.load(std::memory_order_acquire);
    const size_t read_index = m_read_index.load(std::memory_order_relaxed);
    const size_t avail = read_available(write_index, read_index, m_max_size);

    if (avail == 0) {
        return 0;
    }

    const size_t output_count = avail;
    size_t new_read_index = read_index + output_count;

    if (new_read_index > m_max_size) {
        const size_t count0 = m_max_size - read_index;
        const size_t count1 = output_count - count0;

        // TODO : make one at begining and cache.
        void *temp_buffer;
        while((temp_buffer = ::malloc(count0 + count1)) == nullptr)
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        ::memcpy(temp_buffer, (unsigned char *)m_start + read_index, count0);
        ::memcpy((unsigned char *)temp_buffer + count0, (const unsigned char *)m_start, count1);

        m_reader((const unsigned char *)temp_buffer, (count0 + count1));
        ::free(temp_buffer);

        new_read_index -= m_max_size;
    } else {
        m_reader((const unsigned char *)m_start + read_index, output_count);

        if (new_read_index == m_max_size) {
            new_read_index = 0;
        }
    }

    m_read_index.store(new_read_index, std::memory_order_release);
    return output_count;
}
}
