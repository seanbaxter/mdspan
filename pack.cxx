#include <iostream>

template<typename... Ts>
struct tuple {
  // A member pack declaration. Specializes to member names m0, m1, etc.
  Ts ...m;

  template<size_t Index>
  const auto& get() const noexcept {
    return m...[Index];
  }

  template<size_t Index>
  auto& get() noexcept {
    return m...[Index];
  }
};

// Use a deduction guide for CTAD, as aggregate CTAD is not currently
// implemented for member pack declarations.
template<typename... Ts>
tuple(Ts...) -> tuple<Ts...>;

int main() {
  tuple x { 1, 2.2, "Three" };

  std::cout<< "Access with pack subscripts:\n";
  std::cout<< "  "<< x.m...[0]<< "\n";
  std::cout<< "  "<< x.m...[1]<< "\n";
  std::cout<< "  "<< x.m...[2]<< "\n";

  std::cout<< "Access with pack expression:\n";
  std::cout<< "  "<< x.m<< "\n" ...;

  std::cout<< "Access with getter function:\n";
  std::cout<< "  "<< x.get<0>()<< "\n";
  std::cout<< "  "<< x.get<1>()<< "\n";
  std::cout<< "  "<< x.get<2>()<< "\n";

  std::cout<< "Access with injected names:\n";
  std::cout<< "  "<< x.m0<< "\n";
  std::cout<< "  "<< x.m1<< "\n";
  std::cout<< "  "<< x.m2<< "\n";

  std::cout<< "Access with reflection:\n";
  std::cout<< "  "<< decltype(x).member_decl_strings<< ": "<< 
    x.@member_values()<< "\n" ...;
}