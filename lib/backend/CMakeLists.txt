add_executable(${BACKEND} ${BACKEND}.c)

target_include_directories(${BACKEND}
                           PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../..")

if(PORTS)
  install(TARGETS ${BACKEND} DESTINATION ${BACKEND_INSTALL_DIR})
else()
  install(
    TARGETS ${BACKEND}
    PERMISSIONS
      SETUID
      OWNER_WRITE
      OWNER_READ
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
    DESTINATION ${BACKEND_INSTALL_DIR})
endif()
