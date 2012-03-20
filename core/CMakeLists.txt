set(core_SRCS
  sprite.c
  muplex.c
  parser.c
  render.c
  )

add_library(swiff_core ${core_SRCS})
