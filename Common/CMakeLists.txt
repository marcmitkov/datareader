add_library(
        chanel-Common
        STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Factory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Instrument.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Marketable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/StrategyA.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Strategy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Synthetic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Trader.cpp
)
#target_include_directories(chanel-Common PUBLIC "/cygdrive/c/MyProjects/boost_1_75_0")
target_include_directories(chanel-Common PUBLIC "/cygdrive/c/MyHCTech/boost_1_60_0")
target_include_directories(chanel-Common PUBLIC "/cygdrive/c/MyHCTech/chanel-core/src")
target_include_directories(chanel-Common PUBLIC "/cygdrive/c/MyHCTech/Zodiac/include")