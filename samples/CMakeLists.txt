cmake_minimum_required(VERSION 2.8)

project(instantFSM-samples)

include_directories(
  ..
)

enable_testing()

add_executable(audioPlayer audioPlayer.cpp)

add_executable(gameState gameState.cpp)

if (UNIX)
  target_link_libraries(audioPlayer stdc++)
  target_link_libraries(gameState -pthread stdc++)
endif (UNIX)

