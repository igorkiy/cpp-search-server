#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <type_traits>

using namespace std::string_literals;


template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Bucket {
        std::mutex mtx;
        std::map<Key, Value> bucket_map;
    };

    struct Access {
        Access() = default;
        Access(Bucket& bucket, const Key& key) : guard(bucket.mtx), ref_to_value(bucket.bucket_map[key]) {
        }
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
        ~Access() = default;
    };

    ConcurrentMap() = default;
    explicit ConcurrentMap(size_t bucket_count) :
        buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        uint64_t bucket_idx = (static_cast<uint64_t>(key) % buckets_.size());
        return { buckets_[bucket_idx], key };
          
    }
    void Erase(const Key& index) {
        for (auto& item : buckets_) {
            if (item.bucket_map.count(index)) {
                item.mtx.lock(); 
                item.bucket_map.erase(index);
                item.mtx.unlock();
                return;
            }
       }
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& item : buckets_) {
            //блокируем мьютекс текущей корзины
            item.mtx.lock();

            result.insert(item.bucket_map.begin(), item.bucket_map.end());
            item.mtx.unlock();

        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};
