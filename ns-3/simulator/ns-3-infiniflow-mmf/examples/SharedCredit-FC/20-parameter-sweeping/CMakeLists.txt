build_example(
  NAME 20-parameter-sweeping
  SOURCE_FILES 
    20-parameter-sweeping.cc
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