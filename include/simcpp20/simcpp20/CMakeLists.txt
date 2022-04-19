add_library(_simcpp20 INTERFACE)
add_library(simcpp20 ALIAS _simcpp20)

target_include_directories(_simcpp20 INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(_simcpp20 INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10")
    message(WARNING "SimCpp20 requires GCC 10 or later")
  endif()
  target_compile_options(_simcpp20 INTERFACE -fcoroutines)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.28")
    message(WARNING "SimCpp20 requires MSVC 19.28 or later")
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  message(WARNING "Clang compiler has experimental coroutine support")
  target_compile_definitions(_simcpp20 INTERFACE CLANG_COMPILER=1)
endif ()
