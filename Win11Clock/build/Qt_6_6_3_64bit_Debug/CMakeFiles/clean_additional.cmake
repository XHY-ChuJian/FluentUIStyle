# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Win11Clock_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Win11Clock_autogen.dir\\ParseCache.txt"
  "Win11Clock_autogen"
  "dependencies\\ExWidgets\\CMakeFiles\\ExWidgets_autogen.dir\\AutogenUsed.txt"
  "dependencies\\ExWidgets\\CMakeFiles\\ExWidgets_autogen.dir\\ParseCache.txt"
  "dependencies\\ExWidgets\\ExWidgets_autogen"
  )
endif()
