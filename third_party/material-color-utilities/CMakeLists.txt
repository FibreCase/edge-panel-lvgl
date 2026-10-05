add_library(material-color-utilities STATIC
  cpp/cam/cam.cc
  cpp/cam/hct.cc
  cpp/cam/hct_solver.cc
  cpp/cam/viewing_conditions.cc
  cpp/contrast/contrast.cc
  cpp/dislike/dislike.cc
  cpp/dynamiccolor/dynamic_color.cc
  cpp/dynamiccolor/dynamic_scheme.cc
  cpp/dynamiccolor/material_dynamic_colors.cc
  cpp/palettes/tones.cc
  cpp/scheme/scheme_tonal_spot.cc
  cpp/utils/utils.cc
)
target_include_directories(material-color-utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
