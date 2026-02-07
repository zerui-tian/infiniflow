build_example(
  NAME 1-control-dynamics
  SOURCE_FILES 
    1-control-dynamics.cc
    cdf.c
  HEADER_FILES
    cdf.h
  LIBRARIES_TO_LINK
    ${libpoint-to-point}
    ${libtraffic-control}
    ${libinternet}
    ${libapplications}
)

set_source_files_properties(cdf.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)