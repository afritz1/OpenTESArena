#ifndef THREAD_SAFE_MAP_H
#define THREAD_SAFE_MAP_H

#include <unordered_map>
#include <algorithm>

// This does not make guarantees as to the thread safety
// of the values it contains - only of the keys; 
// its main purpose is to provide thread-safe entry creation and iteration.
// Data races can and will happen if the same 
// non-thread-safe value is modified by multiple threads.
template <typename KT, typename VT>
class ThreadSafeMap
{   
private:
    std::unordered_map<KT, VT> entries;
    std::mutex mtx;

public:
    using EntryT = std::pair<KT, VT>;
    
    [[nodiscard]] bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->entries.empty();
    }

    [[nodiscard]] int getSize()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->entries.size();
    }

    void insert(EntryT &&val)
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->entries.insert(std::move(val));
    }   

    VT &operator[](const KT &key)
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->entries[key];
    }
    
    template<class P>
    void forEach(P pred)
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        for(auto &entry: this->entries)
            pred(entry);
    }
};

#endif
