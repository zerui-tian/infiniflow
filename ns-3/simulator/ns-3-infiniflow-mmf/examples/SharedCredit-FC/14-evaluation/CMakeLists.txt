build_example(
  NAME 14-evaluation
  SOURCE_FILES 
    14-evaluation.cc
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