# 梦烬几何 原生层构建脚本
cmake_minimum_required(VERSION 3.5.0)
project(dreamember)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

set(GIAC_DIR ${NATIVERENDER_ROOT_PATH}/giac)

# ---------- giac 静态库（核心 CAS，无 GUI 依赖） ----------
set(GIAC_SRCS
    giac/src/Flv_Data_Source.cc
    giac/src/alg_ext.cc
    giac/src/csturm.cc
    giac/src/derive.cc
    giac/src/desolve.cc
    giac/src/ezgcd.cc
    giac/src/first.cc
    giac/src/gauss.cc
    giac/src/gausspol.cc
    giac/src/gen.cc
    giac/src/global.cc
    giac/src/graphtheory.cc
    giac/src/help.cc
    giac/src/identificateur.cc
    giac/src/ifactor.cc
    giac/src/index.cc
    giac/src/intg.cc
    giac/src/intgab.cc
    giac/src/isom.cc
    giac/src/kadd.cc
    giac/src/kdisplay.cc
    giac/src/lin.cc
    giac/src/misc.cc
    giac/src/modfactor.cc
    giac/src/modpoly.cc
    giac/src/moyal.cc
    giac/src/mp.cc
    giac/src/permu.cc
    giac/src/plot.cc
    giac/src/prog.cc
    giac/src/quater.cc
    giac/src/risch.cc
    giac/src/rpn.cc
    giac/src/series.cc
    giac/src/signalprocessing.cc
    giac/src/solve.cc
    giac/src/sparse.cc
    giac/src/subst.cc
    giac/src/sym2poly.cc
    giac/src/cocoa.cc
    giac/src/pari.cc
    giac/src/symbolic.cc
    giac/src/tinymt32.cc
    giac/src/unary.cc
    giac/src/usual.cc
    giac/src/vecteur.cc
    giac/src/input_lexer.cc
    giac/src/input_parser.cc
    giac/src/caseval.c
    giac/src/mathml.cc
    giac/src/tex.cc
    giac/src/ti89.cc
    giac/src/plot3d.cc
    giac/src/maple.cc
    giac/src/kdisplay.cc
    giac/src/graphe.cc
    giac/src/threaded.cc
    giac/src/cutils.c
    giac/src/libbf.c
    giac/src/libregexp.c
    giac/src/libunicode.c
)

file(GLOB TOMMATH_SRCS giac/tommath/*.c)
add_library(tommath STATIC ${TOMMATH_SRCS})
target_include_directories(tommath PRIVATE ${GIAC_DIR}/include/giac ${GIAC_DIR}/tommath)

add_library(giac STATIC ${GIAC_SRCS})
target_include_directories(giac PRIVATE ${GIAC_DIR}/include ${GIAC_DIR}/include/giac ${GIAC_DIR}/src)
target_compile_definitions(giac PRIVATE
    USE_GMP_REPLACEMENTS=1
    NO_STDEXCEPT=1
    GIAC_NO_GUI=1
    HAVE_CONFIG_H=0
    HAVE_NO_HOME_DIRECTORY=1
    HAVE_LIBPTHREAD=1
    NO_GETOPT=1
)
# giac 源需要 C++11 且允许较多告警
target_compile_options(giac PRIVATE -Wno-everything)

# ---------- dreamember NAPI 模块 ----------
add_library(dreamember SHARED
    napi_init.cpp
    kernel_napi.cpp
    kernel/expr.cpp
    kernel/view.cpp
    kernel/geos.cpp
    kernel/app.cpp
    kernel/ggbfile.cpp
    cas/cas_wrapper.cpp
    cas/ohos_stubs.cpp
)
target_include_directories(dreamember PRIVATE
    ${NATIVERENDER_ROOT_PATH}
    ${GIAC_DIR}/include
    ${GIAC_DIR}/include/giac
)
target_compile_definitions(dreamember PRIVATE WITH_GIAC=1 USE_GMP_REPLACEMENTS=1)
target_link_libraries(dreamember PUBLIC giac tommath libace_napi.z.so libhilog_ndk.z.so z)   # z: sysroot zlib（.ggb zip 容器）
