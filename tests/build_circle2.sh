set -x 
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_contiguous_layouts.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_element_access.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_extents.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_layout_ctors.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_layout_stride.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_mdspan_conversion.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_mdspan_ctors.cpp
circle -P2128 -O0 -I ../circle -lgtest -lgtest_main -lpthread test_submdspan.cpp
