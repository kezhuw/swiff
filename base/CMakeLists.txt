set(base_SRCS
  logger.c
  report.c
  except.c
  assert.c
  bitval.c
  memory.c
  mscope.c
  slab.c
  cxform.c
  )

add_library(swiff_base ${base_SRCS})

install(TARGETS swiff_base DESTINATION lib)

add_executable(logger_unittest logger_test.c)
target_link_libraries(logger_unittest swiff_base)
add_test(base/logger logger_unittest)

add_executable(report_unittest report_test.c)
target_link_libraries(report_unittest swiff_base)
add_test(base/report report_unittest)

add_executable(except_unittest except_test.c)
target_link_libraries(except_unittest swiff_base)
add_test(base/except except_unittest)

add_executable(assert_unittest assert_test.c)
target_link_libraries(assert_unittest swiff_base)
add_test(base/assert assert_unittest)

add_executable(bitval_unittest bitval_test.c)
target_link_libraries(bitval_unittest swiff_base)
add_test(base/bitval bitval_unittest)

add_executable(memory_unittest memory_test.c)
target_link_libraries(memory_unittest swiff_base)
add_test(base/memory memory_unittest)

add_executable(mscope_unittest mscope_test.c)
target_link_libraries(mscope_unittest swiff_base)
add_test(base/mscope memory_unittest)

add_executable(slab_unittest slab_test.c)
target_link_libraries(slab_unittest swiff_base)
add_test(base/slab slab_unittest)
