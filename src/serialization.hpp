#ifndef TASKFLOW_SERIALIZATION_HPP
#define TASKFLOW_SERIALIZATION_HPP

#include <tuple>
#include <functional>
#include <vector>
#include <utility>
#include <cstring>
#include <cassert>
#include <memory>

#include "views.hpp"

namespace tf {

template <std::size_t... Ns, typename... Ts>
constexpr auto tail_impl(std::index_sequence<Ns...>, std::tuple<Ts...> t)
{
    (void)t;
    return std::make_tuple(std::get<Ns + 1u>(t)...);
}

template <typename... Ts>
constexpr auto tail(std::tuple<Ts...> t)
{
    return tail_impl(std::make_index_sequence<sizeof...(Ts) - 1u>(), t);
}

// Allows to call a function for each element of a tuple
template <typename T, typename F, size_t... Is>
constexpr void for_each_impl(T &t, F &&f, std::index_sequence<Is...>)
{
  auto l = {(f(std::get<Is>(t)), 0)...};
  (void)l;
}
template <typename... Ts, typename F>
constexpr void for_each_in_tuple(std::tuple<Ts...> &t, F &&f)
{
  for_each_impl(t, std::forward<F>(f), std::index_sequence_for<Ts...>{});
}

// Serialize data
template <typename... Values>
class Serializer
{
public:
  buffer_t header;
  std::vector<buffer_t> chunks;

private:
    // Computing the header buffer size
    class for_set_size
    {
      Serializer<Values...> &serializer;
    public:
        for_set_size(Serializer<Values...> &serializer_) : serializer(serializer_) {
          serializer.header.size = 0;
          serializer.chunks.clear();
          // 4 bytes for chunk num
          serializer.header.size += sizeof(int);
        }
        template <typename T>
        void operator()(T&)
        {
          serializer.header.size += sizeof(T);
        }
        template <typename T>
        void operator()(view<T> &t)
        {
            int64_t view_size = sizeof(T) * t.size();
            if (view_size < TF_LARGE_MESSAGE_THRESHOLD) {
              serializer.header.size += sizeof(int64_t) + view_size;
            } else {
              // if the view is large, we need to transfer the size information
              serializer.header.size += sizeof(int64_t);
              serializer.chunks.push_back({nullptr, view_size});
            }
        }
    };

    // Write data from tuple to buffer
    class for_write
    {
        Serializer<Values...> &serializer;
        int64_t cpos;
        int chunk_idx;
    public:
        for_write(Serializer<Values...> &serializer_) : serializer(serializer_),
                                                      cpos(0), chunk_idx(0) {
          assert(cpos + sizeof(int) <= serializer.header.size);
          int chunk_num = serializer.chunks.size();
          memcpy((char*)serializer.header.address + cpos, &chunk_num, sizeof(int));
          cpos += sizeof(int);
        }
        template <typename T>
        void operator()(T &t)
        {
            assert(cpos + sizeof(T) <= serializer.header.size);
            memcpy((char*)serializer.header.address + cpos, &t, sizeof(T));
            cpos += sizeof(T);
        }
        template <typename T>
        void operator()(view<T> &t) {
          int view_size = sizeof(T) * t.size();
          assert(cpos + sizeof(int64_t) <= serializer.header.size);
          memcpy((char*)serializer.header.address + cpos, &t.size(), sizeof(int64_t));
          cpos += sizeof(int64_t);
          if (view_size < TF_LARGE_MESSAGE_THRESHOLD) {
            assert(cpos + view_size <= serializer.header.size);
            memcpy((char*)serializer.header.address + cpos, t.data(), view_size);
            cpos += view_size;
          } else {
            buffer_t &chunk = serializer.chunks[chunk_idx++];
            assert(chunk.size = view_size);
            memcpy(chunk.address, t.data(), view_size);
          }
        }
    };

    // Read data from buffer into a tuple
    class for_read
    {
    private:
        Serializer<Values...> &serializer;
        int64_t cpos;
        int chunk_idx;

    public:
        for_read(Serializer<Values...> &serializer_) : serializer(serializer_),
                                                     cpos(sizeof(int)),
                                                     chunk_idx(0) {}
        template <typename T>
        void operator()(T &t)
        {
            assert(cpos + sizeof(T) <= serializer.header.size);
            memcpy(&t, (char*)serializer.header.address + cpos, sizeof(T));
            cpos += sizeof(T);
        }
        template <typename T>
        void operator()(view<T> &t)
        {
            assert(cpos + sizeof(int64_t) <= serializer.header.size);
            int64_t view_num = *(int64_t*) ((char*)serializer.header.address + cpos);
            int64_t view_size = view_num * sizeof(T);
            cpos += sizeof(int64_t);
            if (view_size < TF_LARGE_MESSAGE_THRESHOLD) {
              assert(cpos + view_size <= serializer.header.size);
              t = view<T>((char*)serializer.header.address + cpos, view_num);
              cpos += view_size;
            } else {
              const buffer_t &chunk = serializer.chunks[chunk_idx++];
              assert(chunk.size == view_size);
              t = view<T>(chunk.address, view_num);
            }
        }
    };

public:
    void set_size(Values &&... vals)
    {
        for_set_size sizer(*this);
        auto tup = std::make_tuple(vals...);
        for_each_in_tuple(tup, sizer);
    }

    void write(Values &&... vals)
    {
        auto tup = std::make_tuple(vals...);
        for_each_in_tuple(tup, for_write(*this));
    }

    std::tuple<Values...> read()
    {
        std::tuple<Values...> tup;
        for_each_in_tuple(tup, for_read(*this));
        return tup;
    }
};

//template <>
//class Serializer<>
//{
//public:
//    size_t size() { return 0; };
//    void write(char *){};
//    std::tuple<> read(char *) { return {}; };
//};

} // namespace tf

#endif
