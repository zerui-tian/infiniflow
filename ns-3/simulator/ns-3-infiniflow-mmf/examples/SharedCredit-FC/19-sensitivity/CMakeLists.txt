build_example(
  NAME 19-sensitivity
  SOURCE_FILES 
    19-sensitivity.cc
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