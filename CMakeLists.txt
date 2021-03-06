PROJECT("XDMF_GENERATOR")

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

find_package(HDF5)
if(WIN32)
  link_directories("C:/Program Files/H5FDdsm/lib")
endif()

#--------------------------------------------------
# Load ParaView env
#--------------------------------------------------
IF(NOT USE_H5FD_DSM)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
  INCLUDE(${ParaView_DIR}//VTK/ThirdParty/xdmf2/vtkxdmf2/XDMFConfig.cmake)  
  INCLUDE_DIRECTORIES(${XDMF_INCLUDE_DIRS} ${XDMF_MPI_INCLUDE_PATH} ${XDMF_LIBXML2_INCLUDE})
ENDIF(NOT USE_H5FD_DSM)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump
  ${CMAKE_CURRENT_BINARY_DIR}
  )

SET(XdmfGenerator_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5dump.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5tools.c
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5tools_ref.c
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5tools_str.c
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5tools_type.c
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5tools_utils.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/h5dump/h5trav.c
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfHDFDOM.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/FileSeriesFinder.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfDump.cxx
  )

IF(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
  SET(XdmfGenerator_LIBTYPE SHARED)
ELSE(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
  SET(XdmfGenerator_LIBTYPE STATIC)
ENDIF(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
IF (WIN32)
  SET(XdmfGenerator_LIBTYPE STATIC)
ENDIF (WIN32)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfGeneratorconfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/XdmfGeneratorconfig.h)

ADD_LIBRARY(XdmfGenerator ${XdmfGenerator_LIBTYPE} ${XdmfGenerator_SRCS})
set_target_properties(XdmfGenerator
  PROPERTIES
    COMPILE_FLAGS "-Dhdf5_tools_EXPORTS"
)

IF(USE_H5FD_DSM)
  TARGET_LINK_LIBRARIES(XdmfGenerator H5FDdsm)
ENDIF(USE_H5FD_DSM)
TARGET_LINK_LIBRARIES(XdmfGenerator vtkxdmf2 vtksys)

ADD_EXECUTABLE(XdmfGenerate ${CMAKE_CURRENT_SOURCE_DIR}/XdmfGenerate.cxx)
TARGET_LINK_LIBRARIES(XdmfGenerate XdmfGenerator)

#--------------------------------------------------
# Install
#--------------------------------------------------
INSTALL(
  TARGETS
    XdmfGenerate
  DESTINATION 
    "${CMAKE_INSTALL_PREFIX}/bin"
)

INSTALL(
  TARGETS
    XdmfGenerator
  DESTINATION 
    "${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
)
