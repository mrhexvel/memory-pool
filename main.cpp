#include <iostream>
#include <vector>
#include <mutex>
#include <memory>
#include <cstddef>
#include <stdexcept>

class MemoryPool {
public:
    MemoryPool(std::size_t poolSize) : poolSize(poolSize) {
        pool = static_cast<char*>(std::malloc(poolSize));
        if (!pool) {
            throw std::bad_alloc();
        }
        freeBlocks.push_back(pool);
        freeBlockSizes.push_back(poolSize);
    }

    ~MemoryPool() {
        std::free(pool);
    }

    void* allocate(std::size_t size) {
        std::lock_guard<std::mutex> lock(mtx);

        for (std::size_t i = 0; i < freeBlocks.size(); ++i) {
            if (freeBlockSizes[i] >= size) {
                void* ptr = freeBlocks[i];
                freeBlockSizes[i] -= size;
                if (freeBlockSizes[i] > 0) {
                    freeBlocks[i] = static_cast<char*>(freeBlocks[i]) + size;
                } else {
                    freeBlocks.erase(freeBlocks.begin() + i);
                    freeBlockSizes.erase(freeBlockSizes.begin() + i);
                }
                return ptr;
            }
        }

        throw std::bad_alloc();
    }

    void deallocate(void* ptr, std::size_t size) {
        std::lock_guard<std::mutex> lock(mtx);

        freeBlocks.push_back(static_cast<char*>(ptr));
        freeBlockSizes.push_back(size);

        consolidateFreeBlocks();
    }

private:
    void consolidateFreeBlocks() {
        for (std::size_t i = 0; i < freeBlocks.size(); ++i) {
            for (std::size_t j = i + 1; j < freeBlocks.size(); ++j) {
                if (freeBlocks[i] + freeBlockSizes[i] == freeBlocks[j]) {
                    freeBlockSizes[i] += freeBlockSizes[j];
                    freeBlocks.erase(freeBlocks.begin() + j);
                    freeBlockSizes.erase(freeBlockSizes.begin() + j);
                    --j;
                } else if (freeBlocks[j] + freeBlockSizes[j] == freeBlocks[i]) {
                    freeBlockSizes[j] += freeBlockSizes[i];
                    freeBlocks.erase(freeBlocks.begin() + i);
                    freeBlockSizes.erase(freeBlockSizes.begin() + i);
                    --i;
                    break;
                }
            }
        }
    }

    std::size_t poolSize;
    char* pool;
    std::vector<char*> freeBlocks;
    std::vector<std::size_t> freeBlockSizes;
    std::mutex mtx;
};

int main() {
    MemoryPool pool(1024);

    void* mem1 = pool.allocate(256);
    void* mem2 = pool.allocate(128);

    pool.deallocate(mem1, 256);
    pool.deallocate(mem2, 128);

    std::cout << "память успешно выделена." << std::endl;

    return 0;
}
