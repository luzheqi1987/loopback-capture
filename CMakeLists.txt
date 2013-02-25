cmake_minimum_required (VERSION 2.8) 
project (loopback)

# add_subdirectory(jthread-1.3.1)
add_subdirectory(jrtplib-3.9.1)

include_directories(openssl-1.0.1e/incwin32 jrtplib-3.9.1/src)

set(WIN32_SOURCES
  loopback/win32/raop
  loopback/win32/guid
  loopback/win32/main
  loopback/win32/prefs
  loopback/win32/loopback-capture
)

set(ALAC_SOURCES
  loopback/alac/codec/ag_enc
  loopback/alac/codec/ag_dec
  loopback/alac/codec/ALACBitUtilities
  loopback/alac/codec/ALACEncoder
  loopback/alac/codec/ALACDecoder
  loopback/alac/codec/dp_enc
  loopback/alac/codec/dp_dec
  loopback/alac/codec/EndianPortable
  loopback/alac/codec/matrix_enc
  loopback/alac/codec/matrix_dec
)

set(OPENSSL_LIBS
  ${PROJECT_SOURCE_DIR}/openssl-1.0.1e/out32/libeay32.lib
  ${PROJECT_SOURCE_DIR}/openssl-1.0.1e/out32/ssleay32.lib
)

# set_source_files_properties(ag_dec.c PROPERTIES LANGUAGE CXX)
# set_source_files_properties(loopback/alac/codec/dp_enc.c PROPERTIES LANGUAGE CXX )
# set_source_files_properties(loopback/alac/codec/matrix_enc.c PROPERTIES LANGUAGE CXX )

add_executable(loopback ${WIN32_SOURCES} ${ALAC_SOURCES})
add_definitions(-DUNICODE -D_UNICODE)
target_link_libraries(loopback jrtplib-static ws2_32.lib avrt.lib ${OPENSSL_LIBS})
