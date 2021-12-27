set -x 
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_contiguous_layouts.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_element_access.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_extents.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_layout_ctors.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_layout_stride.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_mdspan_conversion.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_mdspan_ctors.cpp
circle -std=c++20 -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_submdspan.cpp
