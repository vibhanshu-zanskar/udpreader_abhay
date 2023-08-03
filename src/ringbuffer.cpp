#include "ringbuffer.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

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
        m_start = ::mmap(nullptr, rounded_size, ZNS_MMAP_PERMS, ZNS_MMAP_FLAGS, -1, 0);
        if (m_start == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap");
        }
    } else {
        // TODO : posix_memalign?
        m_start = ::malloc(rounded_size);
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
        size_t written_bytes = m_writer(fd, (unsigned char *)m_start + write_index, 131072);

        if ((written_bytes + write_index) > m_max_size) {
            const size_t count0 = m_max_size - write_index;
            const size_t count1 = written_bytes - count0;

            ::memmove(m_start, (unsigned char *)m_end, count1);
            new_write_index = count1;
        } else {
            new_write_index = write_index + written_bytes;
        }
    } else {
        std::size_t written_bytes = m_writer(fd, ((unsigned char *)m_start + write_index), max_bytes);

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

        void *temp_buffer = ::malloc(count0 + count1);
        ::memcpy(temp_buffer, (unsigned char *)m_start + read_index, count0);
        ::memcpy((unsigned char *)temp_buffer + count0, (const unsigned char *)m_start, count1);

        m_reader((const unsigned char *)temp_buffer, (count0 + count1));

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

/*
std::ofstream logFile("log.txt");

std::size_t writePacket(unsigned char *buf, size_t bufLen)
{
    static int64_t cntr = 0;
    cntr++;
    std::string num_to_str = std::to_string(cntr) + '\0';
    if (bufLen < num_to_str.length()) {
        return bufLen;
    }
    ::memcpy(buf, num_to_str.c_str(), num_to_str.length());
    std::cout << "Wrote :" << num_to_str << num_to_str.length() << std::endl;
    return num_to_str.length();
}

std::size_t readPacket(const unsigned char *buf, size_t bufLen)
{
    const char *start;

    for (start = (const char *)buf; start < (const char *)(buf + bufLen);) {
        std::string temp_str = std::string((char *)start);

        std::cout << "Read :" << temp_str << std::endl;
        start += temp_str.length();
    }

    size_t bytes_read = (buf + bufLen - (unsigned char *)start);
    return bytes_read;
}

void pinThread(int cpu)
{
    if (cpu < 0) {
        return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        perror("pthread_setaffinity_no");
        exit(1);
    }
}

#include <thread>

template <typename T> void bench()
{
    const int64_t iters = 100000000;

    T q(256, false, writePacket, readPacket);
    auto t = std::thread([&] {
        pinThread(0);
        for (int i = 0; i < iters; ++i) {
            while (q.pop_all() != 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    pinThread(1);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) {
        while (!q.push(18)) {
        }
    }

    while (q.m_read_index.load(std::memory_order_relaxed) != q.m_write_index.load(std::memory_order_relaxed)) {
    }
    auto stop = std::chrono::steady_clock::now();
    t.join();
    std::cout << iters * 1000000000 / std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count()
              << " ops/s" << std::endl;
}

int main(int argc, char *argv[])
{
    bench<znsreader::RingBuffer>();

    return 0;
}
*/
