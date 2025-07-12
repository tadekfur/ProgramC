# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\EtykietyManager_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\EtykietyManager_autogen.dir\\ParseCache.txt"
  "EtykietyManager_autogen"
  )
endif()
