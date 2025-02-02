set(LLVM_LINK_COMPONENTS
    Support
    Core
    FrontendOpenMP
)

# if(NOT LLVM_ENABLE_RTTI)
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
# endif()

# include(CheckCXXCompilerFlag)
# check_cxx_compiler_flag("-fvisibility-inlines-hidden"
#   SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
# if(${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
# endif()

add_clang_executable(
    refcnt
    refcount.cpp
)

target_link_libraries(
    refcnt
    PRIVATE
    clangASTMatchers
    clangBasic
    clangFrontend
    clangTooling
)