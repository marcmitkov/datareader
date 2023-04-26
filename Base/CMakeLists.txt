add_library(
        chanel-Base
        STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Candle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CandleMaker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PriceInfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Product.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Utilities.cpp
        SymbolInfo.h Position.h Position.cpp CandleMakerEx.cpp CandleMakerEx.h)
target_include_directories(chanel-Base PUBLIC "/cygdrive/c/MyHCTech/boost_1_60_0")
#target_include_directories(chanel-Base PUBLIC "/cygdrive/c/MyProjects/boost_1_75_0")
target_include_directories(chanel-Base PUBLIC "/cygdrive/c/MyHCTech/chanel-core/src")