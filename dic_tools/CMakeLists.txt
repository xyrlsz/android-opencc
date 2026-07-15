cmake_minimum_required(VERSION 3.12)
project(opencc_dict_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCC source root
set(OPENCC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../lib-opencc-android/src/main/jni/OpenCC")

# Marisa library
add_library(marisa STATIC
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/trie.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/agent.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/keyset.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/io/reader.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/io/writer.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/io/mapper.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/trie/louds-trie.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/trie/tail.cc
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib/marisa/grimoire/vector/bit-vector.cc
)
target_include_directories(marisa PRIVATE
    ${OPENCC_ROOT}/deps/marisa-0.3.1/include
    ${OPENCC_ROOT}/deps/marisa-0.3.1/lib
)

# RapidJSON (header-only)
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE
    ${OPENCC_ROOT}/deps/rapidjson-1.1.0
)

# OpenCC core library (minimal subset needed for dict conversion)
set(OPENCC_SRCS
    ${OPENCC_ROOT}/src/BinaryDict.cpp
    ${OPENCC_ROOT}/src/Config.cpp
    ${OPENCC_ROOT}/src/Conversion.cpp
    ${OPENCC_ROOT}/src/ConversionChain.cpp
    ${OPENCC_ROOT}/src/Converter.cpp
    ${OPENCC_ROOT}/src/DartsDict.cpp
    ${OPENCC_ROOT}/src/Dict.cpp
    ${OPENCC_ROOT}/src/DictConverter.cpp
    ${OPENCC_ROOT}/src/DictEntry.cpp
    ${OPENCC_ROOT}/src/DictGroup.cpp
    ${OPENCC_ROOT}/src/Lexicon.cpp
    ${OPENCC_ROOT}/src/MarisaDict.cpp
    ${OPENCC_ROOT}/src/MaxMatchSegmentation.cpp
    ${OPENCC_ROOT}/src/PhraseExtract.cpp
    ${OPENCC_ROOT}/src/PipelineConverter.cpp
    ${OPENCC_ROOT}/src/SerializedValues.cpp
    ${OPENCC_ROOT}/src/SerializableDict.cpp
    ${OPENCC_ROOT}/src/Segmentation.cpp
    ${OPENCC_ROOT}/src/SimpleConverter.cpp
    ${OPENCC_ROOT}/src/SingleStageConverter.cpp
    ${OPENCC_ROOT}/src/TextDict.cpp
    ${OPENCC_ROOT}/src/UTF8StringSlice.cpp
    ${OPENCC_ROOT}/src/UTF8Util.cpp
    ${OPENCC_ROOT}/src/PluginSegmentation.cpp
    ${OPENCC_ROOT}/src/PrefixMatch.cpp
    ${OPENCC_ROOT}/src/ResourceProvider.cpp
    ${OPENCC_ROOT}/src/tools/PlatformIO.cpp
)

add_library(libopencc_host STATIC ${OPENCC_SRCS})
target_include_directories(libopencc_host PUBLIC
    ${OPENCC_ROOT}                    # for #include "src/..."
    ${OPENCC_ROOT}/src                # for #include "Conversion.hpp" etc.
    ${OPENCC_ROOT}/deps/rapidjson-1.1.0
    ${OPENCC_ROOT}/deps/marisa-0.3.1/include
    ${OPENCC_ROOT}/deps/darts-clone-0.32h/include   # for darts.h
    ${OPENCC_ROOT}/deps/tclap-1.2.5                  # for tclap/CmdLine.h
)
target_link_libraries(libopencc_host PUBLIC marisa rapidjson)
target_compile_definitions(libopencc_host PUBLIC
    OPENCC_ENABLE_DARTS
    Opencc_BUILT_AS_STATIC         # 静态库：OPENCC_EXPORT 置空
)
target_compile_options(libopencc_host PRIVATE /D_CRT_SECURE_NO_WARNINGS)

# opencc_dict executable
add_executable(opencc_dict
    ${OPENCC_ROOT}/src/tools/DictConverter.cpp
)
target_include_directories(opencc_dict PRIVATE
    ${OPENCC_ROOT}
    ${OPENCC_ROOT}/src
    ${OPENCC_ROOT}/deps/tclap-1.2.5
)
target_link_libraries(opencc_dict libopencc_host)
