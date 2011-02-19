set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

find_package(Threads REQUIRED)

add_library (actor SHARED actor.c list.c)
  set_target_properties(actor PROPERTIES VERSION 0.0.1 SOVERSION 1)
  install(TARGETS actor DESTINATION ${CMAKE_INSTALL_LIBDIR})
  target_link_libraries(actor ${CMAKE_THREAD_LIBS_INIT})

install(FILES actor.h list.h types.h DESTINATION include/libactor)