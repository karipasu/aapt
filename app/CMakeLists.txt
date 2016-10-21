# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             src/main/cpp/native-lib.cpp
             src/main/cpp/utils-cpp/Asset.cpp
             src/main/cpp/utils-cpp/AssetManager.cpp
             src/main/cpp/utils-cpp/atomic.cpp
             src/main/cpp/utils-cpp/BufferedTextOutput.cpp
             src/main/cpp/utils-cpp/Debug.cpp
             src/main/cpp/utils-cpp/FileMap.cpp
             src/main/cpp/utils-cpp/misc.cpp
             src/main/cpp/utils-cpp/RefBase.cpp
             src/main/cpp/utils-cpp/ResourceTypes.cpp
             src/main/cpp/utils-cpp/SharedBuffer.cpp
             src/main/cpp/utils-cpp/TextOutput.cpp
             src/main/cpp/utils-cpp/Static.cpp
             src/main/cpp/utils-cpp/StreamingZipInflater.cpp
             src/main/cpp/utils-cpp/String16.cpp
             src/main/cpp/utils-cpp/String8.cpp
             src/main/cpp/utils-cpp/threadsex.c
             src/main/cpp/utils-cpp/Unicode.cpp
             src/main/cpp/utils-cpp/VectorImpl.cpp
             src/main/cpp/utils-cpp/ZipFileRO.cpp
             src/main/cpp/utils-cpp/ZipUtils.cpp
             src/main/cpp/zlib/adler32.c
             src/main/cpp/zlib/compress.c
             src/main/cpp/zlib/crc32.c
             src/main/cpp/zlib/deflate.c
             src/main/cpp/zlib/gzclose.c
             src/main/cpp/zlib/gzlib.c
             src/main/cpp/zlib/gzread.c
             src/main/cpp/zlib/gzwrite.c
             src/main/cpp/zlib/infback.c
             src/main/cpp/zlib/inffast.c
             src/main/cpp/zlib/inflate.c
             src/main/cpp/zlib/inftrees.c
             src/main/cpp/zlib/trees.c
             src/main/cpp/zlib/uncompr.c
             src/main/cpp/zlib/zutil.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
