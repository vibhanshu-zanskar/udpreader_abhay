#ifndef __ZNS_RING_BUFFER_H
#define __ZNS_RING_BUFFER_H

#include <atomic>
#include <functional>
#include <sys/mman.h>

#define ZNS_MMAP_FLAGS (MAP_HUGETLB | MAP_ANONYMOUS | MAP_SHARED)
#define ZNS_MMAP_PERMS (PROT_READ | PROT_WRITE)

namespace znsreader
{
class RingBuffer
{
    using WriterCallBack = std::function<::size_t(unsigned char *data, size_t size)>;
    using ReaderCallBack = std::function<::size_t(const unsigned char *data, size_t size)>;

  public:
    RingBuffer() = delete;
    RingBuffer(std::size_t max_size, bool use_huge_pages, WriterCallBack writer_fn, ReaderCallBack reader_fn);
    ~RingBuffer();
    ::ssize_t free_space();
    std::size_t push(std::size_t max_bytes);
    std::size_t pop_all();

  protected:
    static inline size_t write_available(size_t write_index, size_t read_index, size_t max_size)
    {
        size_t ret = read_index - write_index - 1;
        if (write_index >= read_index) {
            ret += max_size;
        }

        return ret;
    }

    static inline size_t read_available(size_t write_index, size_t read_index, size_t max_size)
    {
        if (write_index >= read_index) {
            return write_index - read_index;
        }

        const size_t ret = write_index + max_size - read_index;
        return ret;
    }

    static inline size_t align_size(size_t max_size, bool huge_page)
    {
        size_t page_size = (1 << 12); // 4K
        if (huge_page) {
            page_size = (1 << 21); // 2MB
        }

        // TODO : Find a way to include ARG_MAX
        return ((max_size) + (page_size - 1)) & ~(page_size - 1);
    }

  private:
    alignas(64) std::atomic<size_t> m_read_index;
    ReaderCallBack m_reader;
    WriterCallBack m_writer;
    volatile void *m_start;
    void *m_end;
    size_t m_max_size;
    size_t m_allocated_size;
    bool m_use_huge_pages;
    alignas(64) std::atomic<size_t> m_write_index;
};

}
#endif
