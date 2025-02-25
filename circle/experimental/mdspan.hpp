#if !defined(__circle_lang__) || (__circle_build__ < 146)
  #error Must compile with Circle build 146 or later
#endif

#include <cassert>
#include <stdexcept>
#include <tuple>
#include <array>
#include <limits>
#include <type_traits>

namespace std {
namespace experimental {

using std::array;

constexpr size_t dynamic_extent = size_t.max;

// [mdspan.extents], class template extents
template<size_t... Extents>
class extents;

// Spam out dynamic_extent Rank times.
template<size_t Rank>
using dextents = extents<for i : Rank => dynamic_extent>;

template<typename>
constexpr size_t make_dynamic_extent() { return dynamic_extent; } 

// The Index template parameter is required so that each data member is 
// of a different type, allowing the [[no_unique_address]] attribute
// to map them all to the same struct offset.
template<typename Type>
concept SizeType = std::is_convertible_v<Type, size_t>;

template<size_t Index, size_t Extent>
struct _storage_t {
  constexpr _storage_t(size_t value = Extent) noexcept { 
    assert(value == extent);
  }
  static constexpr size_t extent = Extent;
};

template<size_t Index>
struct _storage_t<Index, dynamic_extent> {
  constexpr _storage_t(size_t value = 0) noexcept : extent(value) { }
  size_t extent;
};

template<size_t... Extents>
struct extents {
  using size_type = size_t;

  static constexpr size_t rank() noexcept { return sizeof... Extents; }
  static constexpr size_t rank_dynamic() noexcept {
    return Extents.count(dynamic_extent);
  }
  static constexpr size_t static_extent(size_t n) noexcept {
    return n == int... ...? Extents : 0;
  }

  // Set the runtime extents to 1.
  constexpr extents() = default;

  template<size_t... OtherExtents>
  requires((... && 
    (
      Extents == dynamic_extent ||
      OtherExtents == dynamic_extent ||
      Extents == OtherExtents
    )
  ))
  explicit((... ||
    (
      Extents != dynamic_extent &&
      OtherExtents == dynamic_extent
    )
  ))
  constexpr extents(const extents<OtherExtents...>& other) :
    m(other. ...m.extent)... { }

  // One index per extent.
  template<SizeType... IndexTypes>
  requires(sizeof...(IndexTypes) == rank())
  explicit constexpr extents(IndexTypes... exts) noexcept :
    m(exts)... { }

  // Map index I to dynamic index J.
  template<size_t I>
  static constexpr size_t find_dynamic_index = 
    Extents...[:I].count(dynamic_extent);

  // One index per dynamic extent.
  template<SizeType... IndexTypes> requires(
    sizeof...(IndexTypes) != rank() && 
    sizeof...(IndexTypes) == rank_dynamic()
  )
  explicit constexpr extents(IndexTypes... exts) noexcept : m(
    dynamic_extent == Extents ??
      exts...[find_dynamic_index<int...>] : 
      Extents
  )... { }

  template<SizeType IndexType, size_t N>
  requires(N == rank() || N == rank_dynamic())
  explicit(N != rank_dynamic())
  constexpr extents(const std::array<IndexType, N>& exts) noexcept :
    extents(exts...) { }

  template<size_t... Extents2>
  friend constexpr bool operator==(const extents& lhs, 
    const extents<Extents2...>& rhs) noexcept {
    return sizeof... Extents2 == rank() &&& 
      (... && (lhs.get<int...(rank())>() == rhs.template get<int...>()));
  }

  template<int I>
  constexpr size_t get() const noexcept {
    return m...[I].extent;
  }

  constexpr size_t extent(size_t n) const noexcept {
    return n == int... ...? m.extent : 0;
  }

  // Create partially static storage for the dynamic extents.
  [[no_unique_address]] _storage_t<int..., Extents> ...m;
};

template <typename... SizeTypes>
extents(SizeTypes...) -> extents<make_dynamic_extent<SizeTypes>()...>;

struct layout_left;
struct layout_right;
struct layout_stride;

struct layout_right {
  template <class Extents>
  class mapping {
  private:
    static_assert(extents == Extents.template, 
      "layout_right::mapping must be instantiated with extents");

    template <class>
    friend class mapping;

    template<typename... Indices>
    constexpr size_t compute_offset(Indices... indices) const noexcept {
      // The right-most extent is the most quickly varying.
      size_t x = 0;
      @meta for(int i : sizeof... indices) {
        @meta if(i != 0)
          x *= _extents.template get<i>();
        x += indices...[i];
      }
      return x;
    }

  public:
    constexpr mapping() noexcept = default;
    constexpr mapping(mapping const&) noexcept = default;
    constexpr mapping(mapping&&) noexcept = default;
    constexpr mapping& operator=(mapping const&) noexcept = default;
    constexpr mapping& operator=(mapping&&) noexcept = default;

    using layout_type = layout_right;
    using extents_type = Extents;
    using size_type = typename Extents::size_type;

    constexpr mapping(const Extents& extents) noexcept
      : _extents(extents) { }

    template<typename OtherExtents>
    requires(std::is_constructible_v<Extents, OtherExtents>)
    explicit(!std::is_convertible_v<OtherExtents, Extents>)
    constexpr mapping(const mapping<OtherExtents>& other) noexcept :
      _extents(other.extents()) { }

    // Convert from layout_left
    template<typename OtherMapping>
    requires(
      std::is_constructible_v<Extents, typename OtherMapping::extents_type> &&
      typename OtherMapping::layout_type == layout_left &&
      Extents::rank() <= 1
    )
    explicit(!std::is_convertible_v<typename OtherMapping::extents_type, Extents>)
    constexpr mapping(const OtherMapping& other) noexcept :
      _extents(other.extents()) { }

    // Convert from layout_stride
    template<typename OtherMapping>
    requires(
      std::is_constructible_v<Extents, typename OtherMapping::extents_type> &&
      typename OtherMapping::layout_type == layout_stride
    )
    explicit(Extents::rank() != 0)
    constexpr mapping(const OtherMapping& other) noexcept :
      _extents(other.extents()) {

      size_t stride = 1;
      for(size_t r = _extents.rank(); r > 0; --r) {
        if(stride != other.stride(r - 1))
          throw std::runtime_error(
            "Assigning layout_stride to layout_right with invalid strides.");
        stride *= _extents.extent(r - 1);
      }
    }

    template<typename... Indices>
    constexpr size_t operator()(Indices... idx) const noexcept {
      return compute_offset(idx...);
    }

    constexpr Extents extents() const noexcept {
      return _extents;
    }

    constexpr size_t stride(size_t i) const noexcept {
      size_t value = 1;
      for(size_t r = Extents::rank() - 1; r > i; --r)
        value *= _extents.extent(r);
      return value;
    }

    constexpr size_t required_span_size() const noexcept {
      return (1 * ... * _extents.template get<int...(Extents::rank())>());
    }

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_contiguous() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
    constexpr bool is_unique() const noexcept { return true; }
    constexpr bool is_contiguous() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return true; }

    template<class OtherExtents>
    friend constexpr bool operator==(const mapping& lhs, 
      const mapping<OtherExtents>& rhs) noexcept {
      return lhs.extents() == rhs.extents();
    }

  private:
    [[no_unique_address]] Extents _extents;
  };
};

struct layout_left {
  template <class Extents>
  class mapping {
    static_assert(Extents.template == extents, 
        "layout_left::mapping must be instantiated with extents.");

    template <class>
    friend class mapping;

    template<typename... Indices>
    constexpr size_t compute_offset(Indices... indices) const noexcept {
      // The left-most extent is most quickly varying.
      size_t x = 0;
      @meta for(int i = sizeof... indices - 1; i >= 0; --i) {
        if constexpr(i != sizeof... indices - 1)
          x *= _extents.template get<i>();
        x += indices...[i];
      }
      return x;
    }

  public:
    constexpr mapping() noexcept = default;
    constexpr mapping(mapping const&) noexcept = default;
    constexpr mapping(mapping&&) noexcept = default;
    mapping& operator=(mapping const&) noexcept = default;
    mapping& operator=(mapping&&) noexcept = default;

    using layout_type = layout_left;
    using extents_type = Extents;
    using size_type = typename Extents::size_type;

    constexpr mapping(const Extents& exts) noexcept
      : _extents(exts) { }

    template<typename OtherExtents>
    requires(std::is_constructible_v<Extents, OtherExtents>)
    explicit(!std::is_convertible_v<OtherExtents, Extents>)
    constexpr mapping(const mapping<OtherExtents>& other) noexcept :
      _extents(other.extents()) { }

    // Convert from layout_right.
    template<typename OtherMapping>
    requires(
      std::is_constructible_v<Extents, typename OtherMapping::extents_type> &&
      typename OtherMapping::layout_type == layout_right &&
      Extents::rank() <= 1
    )
    explicit(!std::is_convertible_v<typename OtherMapping::extents_type, Extents>)
    constexpr mapping(const OtherMapping& other) noexcept :
      _extents(other.extents()) { }

    // Convert from layout_stride
    template<typename OtherMapping>
    requires(
      std::is_constructible_v<Extents, typename OtherMapping::extents_type> &&
      typename OtherMapping::layout_type == layout_stride
    )
    explicit(Extents::rank() != 0)
    constexpr mapping(const OtherMapping& other) noexcept :
      _extents(other.extents()) {

      size_t stride = 1;
      for(size_type r = 0; r < _extents.rank(); ++r) {
        if(stride != other.stride(r))
          throw std::runtime_error(
            "Assigning layout_stride to layout_left with invalid strides.");
        stride *= _extents.extent(r);
      }
    }

    template <typename... Indices>
    constexpr size_type operator()(Indices... idxs) const noexcept {
      return compute_offset(idxs...);
    }

    constexpr Extents extents() const noexcept {
      return _extents;
    }

    constexpr size_type stride(size_t i) const noexcept {
      size_t value = 1;
      for(size_t r = 0; r < i; ++r) 
        value *= _extents.extent(r);
      return value;
    }

    constexpr size_type required_span_size() const noexcept {
      return (1 * ... * _extents.template get<int...(Extents::rank())>());
    }

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_contiguous() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }

    constexpr bool is_unique() const noexcept { return true; }
    constexpr bool is_contiguous() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return true; }

    template<class OtherExtents>
    friend constexpr bool operator==(const mapping& lhs, 
      const mapping<OtherExtents>& rhs) noexcept {
      return lhs.extents() == rhs.extents();
    }

  private:
    [[no_unique_address]] Extents _extents;
  };
};

struct layout_stride {
  template <class Extents>
  class mapping {
  public:
    static_assert(Extents.template == extents, 
        "layout_string::mapping must be instantiated with extents.");

    using size_type = typename Extents::size_type;
    using extents_type = Extents;
    using layout_type = layout_stride;

  private:
    //----------------------------------------------------------------------------

    template <class>
    friend class mapping;

    template<typename... Indices>
    constexpr size_t compute_index(Indices... indices) const noexcept {
      return (0 + ... + (indices * _strides[int...]));
    }

    template<typename OtherMapping>
    static constexpr std::array<size_t, Extents::rank()> 
    fill_storage(const OtherMapping& other) noexcept {
      return { other.stride(int...(Extents::rank()))... };
    }

  public: 
    constexpr mapping() noexcept = default;
    constexpr mapping(mapping const&) noexcept = default;
    constexpr mapping(mapping&&) noexcept = default;
    constexpr mapping& operator=(mapping const&) noexcept = default;
    constexpr mapping& operator=(mapping&&) noexcept = default;

    template<SizeType StrideType>
    constexpr mapping(const Extents& exts, 
      const std::array<StrideType, Extents::rank()>& strides) noexcept :
      _extents(exts), _strides { strides... } { }

    template<typename OtherExtents>
    explicit(!std::is_convertible_v<OtherExtents, Extents>)
    constexpr mapping(const mapping<OtherExtents>& rhs) noexcept :
      _extents(rhs.extents()), _strides(rhs._strides) { }


    template<typename OtherMapping>
    requires(
      std::is_constructible_v<Extents, typename OtherMapping::extents_type> &&
      typename OtherMapping::layout_type::template mapping<typename OtherMapping::extents_type> == OtherMapping,
      OtherMapping::is_always_unique() &&
      OtherMapping::is_always_strided()
    )
    explicit(!std::is_convertible_v<typename OtherMapping::extents_type, Extents>)
    constexpr mapping(const OtherMapping& other) noexcept :
      _extents(other.extents()), _strides(fill_storage(other)) { }

    constexpr extents_type extents() const noexcept {
      return _extents;
    };

    constexpr bool is_unique() const noexcept { return true; }
    constexpr bool is_contiguous() const noexcept {
      // TODO: Iplementat once I understand what this means.
      return false;
    }
    constexpr bool is_strided() const noexcept { return true; }

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_contiguous() noexcept {
      return false;
    }
    static constexpr bool is_always_strided() noexcept { return true; }

    template<SizeType... Indices>
    requires(sizeof...(Indices) == Extents::rank())
    constexpr size_t operator()(Indices... indices) const noexcept {
      return compute_index(indices...);
    }

    constexpr size_t stride(size_t r) const noexcept {
      return _strides[r];
    }

    constexpr const array<size_t, Extents::rank()>& strides() const noexcept {
      return _strides;
    }

    constexpr size_t required_span_size() const noexcept {
      // Return 0 if any extent is zero.
      if((... || !_extents.template get<int...(Extents::rank())>()))
        return 0;

      // Return the largest stride * extent product.
      return std::max({ 1zu, _extents.template get<int...>() * _strides.[:] ... });
    }

    template<class OtherExtents>
    requires(OtherExtents::rank() == Extents::rank())
    friend constexpr bool operator==(const mapping& lhs, 
      const mapping<OtherExtents>& rhs) noexcept {
      return (... && (lhs.stride(int...(Extents::rank())) == rhs.stride(int...)));
    }

    [[no_unique_address]] Extents _extents;
    [[no_unique_address]] std::array<size_t, Extents::rank()> _strides;
  };
};

////////////////////////////////////////////////////////////////////////////////

template <class ElementType>
struct default_accessor {
  using offset_policy = default_accessor;
  using element_type = ElementType;
  using reference = ElementType&;
  using pointer = ElementType*;

  constexpr default_accessor() noexcept = default;

  template<typename OtherElementType>
  requires(std::is_convertible_v<OtherElementType(*)[], element_type(*)[]>)
  constexpr default_accessor(default_accessor<OtherElementType>) noexcept { }

  constexpr pointer offset(pointer p, size_t i) const noexcept {
    return p + i;
  }

  constexpr reference access(pointer p, size_t i) const noexcept {
    return p[i];
  }
};

////////////////////////////////////////////////////////////////////////////////

template <
  class ElementType,
  class Extents,
  class Layout = layout_right,
  class Accessor = default_accessor<ElementType>
>
class mdspan {
  static_assert(extents == Extents.template, 
    "Extents parameter must be a specialization of extents");

public:
  using extents_type = Extents;
  using layout_type = Layout;
  using accessor_type = Accessor;
  using mapping_type = typename layout_type::template mapping<extents_type>;
  using element_type = ElementType;
  using value_type = element_type.remove_cv;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = typename accessor_type::pointer;
  using reference = typename accessor_type::reference;

  constexpr mdspan() = default;
  constexpr mdspan(const mdspan&) = default;
  constexpr mdspan(mdspan&&) = default;

  template<SizeType... IndexTypes>
  requires(
    std::is_constructible_v<extents_type, IndexTypes...> &&
    std::is_constructible_v<mapping_type, extents_type> &&
    std::is_default_constructible_v<accessor_type>
  )
  explicit constexpr mdspan(pointer p, IndexTypes... dynamic_extents) : 
    _pointer(p), _mapping(extents_type(dynamic_extents...)) { }

  template<SizeType IndexType, size_t N>
  requires(
    std::is_constructible_v<extents_type, array<IndexType, N> > &&
    std::is_constructible_v<mapping_type, extents_type> &&
    std::is_default_constructible_v<accessor_type>
  ) 
  explicit(N != extents_type::rank_dynamic())
  constexpr mdspan(pointer p, const array<IndexType, N>& dynamic_extents)
    : _pointer(p), _mapping(extents_type(dynamic_extents)) { }

  constexpr mdspan(pointer p, const extents_type& exts) requires(
    std::is_constructible<mapping_type, extents_type> &&
    std::is_default_constructible_v<accessor_type>
  ) : _pointer(p), _mapping(exts) { }

  constexpr mdspan(pointer p, const mapping_type& m) 
  requires(std::is_default_constructible_v<accessor_type>) : 
    _pointer(p), _mapping(m) { }

  constexpr mdspan(pointer p, const mapping_type& m, const accessor_type& a)
    : _pointer(p), _mapping(m), _accessor(a) { }

  template<
    typename ElementType2, typename Extents2, 
    typename Layout2, typename Accessor2
  > requires(
    std::is_constructible_v<mapping_type, typename Layout2::template mapping<Extents2> > &&
    std::is_constructible_v<accessor_type, Accessor2> &&
    std::is_constructible_v<pointer, typename Accessor2::pointer> &&
    std::is_constructible_v<extents_type, Extents2>
  )
  constexpr mdspan(const mdspan<ElementType2, Extents2, Layout2, Accessor2>& other) :
    _pointer(other._pointer), _mapping(other._mapping), 
    _accessor(other._accessor) { }

  // Indexing.
  template<SizeType... IndexTypes>
  requires(extents_type::rank() == sizeof...(IndexTypes))
  constexpr reference operator[](IndexTypes... indices) const noexcept {
    return _accessor.access(_pointer, _mapping(indices...));
  }

  template<SizeType IndexType, size_t N>
  requires(N == extents_type::rank())
  constexpr reference operator[](const array<IndexType, N>& indices) const noexcept {
    return _accessor.access(_pointer, _mapping(indices...));
  }

  template<SizeType... IndexTypes>
  requires(extents_type::rank() == sizeof...(IndexTypes))
  constexpr reference operator()(IndexTypes... indices) const noexcept {
    return _accessor.access(_pointer, _mapping(indices...));
  }

  template<SizeType IndexType, size_t N>
  requires(N == extents_type::rank())
  constexpr reference operator()(const array<IndexType, N>& indices) const noexcept {
    return _accessor.access(_pointer, _mapping(indices...));
  }

  constexpr accessor_type accessor() const { return _accessor; };

  static constexpr size_t rank() noexcept { return extents_type::rank(); }
  static constexpr size_t rank_dynamic() noexcept { return extents_type::rank_dynamic(); }
  static constexpr size_type static_extent(size_t r) noexcept { return extents_type::static_extent(r); }

  constexpr extents_type extents() const noexcept { return _mapping.extents(); };
  constexpr size_type extent(size_t r) const noexcept { return _mapping.extents().extent(r); };
  constexpr size_type size() const noexcept {
    return (1 * ... * extents().template get<int...(extents_type::rank())>);
  };

  constexpr pointer data() const noexcept { return _pointer; };

  static constexpr bool is_always_unique() noexcept { return mapping_type::is_always_unique(); };
  static constexpr bool is_always_contiguous() noexcept { return mapping_type::is_always_contiguous(); };
  static constexpr bool is_always_strided() noexcept { return mapping_type::is_always_strided(); };

  constexpr mapping_type mapping() const noexcept { return _mapping; }
  constexpr bool is_unique() const noexcept { return _mapping.is_unique(); };
  constexpr bool is_contiguous() const noexcept { return _mapping.is_contiguous(); };
  constexpr bool is_strided() const noexcept { return _mapping.is_strided(); };
  constexpr size_type stride(size_t r) const { return _mapping.stride(r); };

private:

  pointer _pointer;
  [[no_unique_address]] mapping_type _mapping;
  [[no_unique_address]] accessor_type _accessor;

  template <class, class, class, class>
  friend class mdspan;
};

template<typename ElementType, SizeType... IndexTypes>
mdspan(ElementType*, IndexTypes...)
  -> mdspan<ElementType, extents<make_dynamic_extent<IndexTypes>()...>>;

template <class ElementType, typename IndexType, size_t N>
mdspan(ElementType*, const std::array<IndexType, N>&)
  -> mdspan<ElementType, dextents<N>>;

template <class ElementType, size_t... ExtentsPack>
mdspan(ElementType*, const extents<ExtentsPack...>&)
  -> mdspan<ElementType, extents<ExtentsPack...>>;

template <class ElementType, class MappingType>
mdspan(ElementType*, const MappingType&)
  -> mdspan<ElementType, typename MappingType::extents_type, typename MappingType::layout_type>;

template <class MappingType, class AccessorType>
mdspan(const typename AccessorType::pointer, const MappingType&, const AccessorType&)
  -> mdspan<typename AccessorType::element_type, typename MappingType::extents_type, typename MappingType::layout_type, AccessorType>;


struct full_extent_t { explicit full_extent_t() = default; };
inline constexpr auto full_extent = full_extent_t{ };

// submdspan
template<typename ET, size_t... Exts, typename LP, typename AP, typename... SliceSpecs>
requires(
  (LP == layout_left || LP == layout_right || LP == layout_stride) && (
    (... && (
      std::is_convertible_v<SliceSpecs, size_t> ||
      std::is_convertible_v<SliceSpecs, std::tuple<size_t, size_t>> ||
      std::is_convertible_v<SliceSpecs, full_extent_t>
    ))
  )
)
constexpr auto submdspan(const mdspan<ET, extents<Exts...>, LP, AP>& src,
  SliceSpecs... slices) {
 
  using tuple = std::tuple<size_t, size_t>;
  constexpr size_t rank = src.rank();

  // * For size_t slice, skip the extent and stride, but add an offset
  //   corresponding to the value.
  // * For a std::full_extent, offset 0 and old extent.
  // * For a std::tuple, add an offset and add a new dynamic extent 
  //   (strides still preserved).
  size_t offset = src.mapping()(
    std::is_convertible_v<SliceSpecs, size_t> ??
      (size_t)slices :
    std::is_convertible_v<SliceSpecs, tuple> ??
      std::get<0>((tuple)slices) :
      0 ...
  );

  using Extents = extents<
    if std::is_convertible_v<SliceSpecs, tuple> => dynamic_extent else
    if std::is_convertible_v<SliceSpecs, full_extent_t> => Exts ...
  >; 
  constexpr size_t rank2 = Extents::rank();

  Extents sub_extents {
    if std::is_convertible_v<SliceSpecs, tuple> => 
      std::get<1>((tuple)slices) - std::get<0>((tuple)slices) 
    else if std::is_convertible_v<SliceSpecs, full_extent_t> =>
      src.extent(int...) ...
  };

  // If LayoutPolicy is layout_left and sub.rank() > 0 is true, then:
  // if is_convertible_v<Sk,full_extent_t> is true for all k in the range
  //   [0, sub.rank() - 1) and is_convertible_v<Sk,size_t> is false for k equal
  //   sub.rank()-1, then decltype(sub)::layout_type is layout_left.
  constexpr bool is_layout_left = layout_left == LP &&& (!rank2 |||
    (... && std::is_convertible_v<SliceSpecs...[0:rank2 - 1], full_extent_t>) &&&
    !std::is_convertible_v<SliceSpecs...[rank2 - 1], size_t>
  );

  // If LayoutPolicy is layout_right and sub.rank() > 0 is true, then:
  // if is_convertible_v<Sk,full_extent_t> is true for all k in the range
  //   [src.rank()-sub.rank()+1,src.rank()) and is_convertible_v<Sk,size_t>
  //   is false for k equal src.rank()-sub.rank(), 
  //   then decltype(sub)::layout_type is layout_right.
  constexpr size_t is_layout_right = layout_right == LP &&& (!rank2 |||
    (... && std::is_convertible_v<SliceSpecs...[rank - rank2 + 1:rank], full_extent_t>) &&&
    !std::is_convertible_v<SliceSpecs...[rank - rank2], size_t>
  );

  if constexpr(is_layout_left || is_layout_right) {
    // Use the source's layout policy.
    return mdspan<ET, Extents, LP, AP>(
      src.data() + offset,
      typename LP::template mapping<Extents>(sub_extents),
      src.accessor()
    );

  } else {
    // Use layout_stride policy.   
    std::array<size_t, Extents::rank()> strides {
      if !std::is_convertible_v<SliceSpecs, size_t> => src.stride(int...) ...
    };

    return mdspan<ET, Extents, layout_stride, AP>(
      src.data() + offset,
      layout_stride::mapping<Extents>(sub_extents, strides),
      src.accessor()
    );
  }
}

}
}

// Kokkos mdspan macros to let the Kokkos tests work.

#define _MDSPAN_INLINE_VARIABLE inline
#define MDSPAN_INLINE_FUNCTION inline
#define __MDSPAN_OP(mds,...) mds[__VA_ARGS__]
#define __MDSPAN_OP0(mds) mds[]
#define _MDSPAN_HOST_DEVICE
#define _MDSPAN_CPLUSPLUS __cplusplus

#define MDSPAN_CXX_STD_14 201402L
#define MDSPAN_CXX_STD_17 201703L
#define MDSPAN_CXX_STD_20 202002L

#define MDSPAN_HAS_CXX_14 1
#define MDSPAN_HAS_CXX_17 1
#define MDSPAN_HAS_CXX_20 1

#define _MDSPAN_USE_CONCEPTS 1
#define _MDSPAN_USE_FOLD_EXPRESSIONS 1
#define _MDSPAN_USE_INLINE_VARIABLES 1
#define _MDSPAN_USE_VARIABLE_TEMPLATES 1
#define _MDSPAN_USE_INTEGER_SEQUENCE 1
#define _MDSPAN_USE_RETURN_TYPE_DEDUCTION 1

#define _MDSPAN_USE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION 1
// #define _MDSPAN_USE_ALIAS_TEMPLATE_ARGUMENT_DEDUCTION 1
