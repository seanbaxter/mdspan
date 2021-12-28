#include <type_traits>
#include <limits>
#include <iostream>

constexpr size_t dynamic_extent = size_t.max;

template<typename Type>
concept SizeType = std::is_convertible_v<Type, size_t>;

template<size_t index, size_t Extent>
struct _storage_t {
  // static storage.
  constexpr _storage_t(size_t extent) noexcept { }
  static constexpr size_t extent = Extent;
};

template<size_t index>
struct _storage_t<index, dynamic_extent> {
  // dynamic storage.
  constexpr _storage_t(size_t extent) noexcept : extent(extent) { }
  size_t extent;
};

template<size_t... Extents>
struct extents {
  template<SizeType... IndexTypes>
  constexpr extents(IndexTypes... indices) noexcept : m(indices)... { }

  constexpr size_t extent(size_t i) const noexcept {
    return i == int... ...? m.extent : 0;
  }

  // Partial static storage.
  [[no_unique_address]] _storage_t<int..., Extents> ...m;
};

int main() {
  extents<3, 4, dynamic_extent, dynamic_extent, 7> m(3, 4, 5, 6, 7);

  // Only use 8 bytes per *dynamic* extent.
  static_assert(2 * sizeof(size_t) == sizeof(m));

  // Print out each extent, dynamic or static.
  for(int i : 5)
    std::cout<< m.extent(i)<< "\n";
}