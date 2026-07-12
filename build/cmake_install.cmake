# Install script for directory: D:/SOOBSHESTVA/VPN/VPN-TEIVRIM

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/VPN-TEIVRIM")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/mingw64/bin/objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/build/VPN-TEIVRIM.exe")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/VPN-TEIVRIM.exe" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/VPN-TEIVRIM.exe")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "C:/mingw64/bin/strip.exe" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/VPN-TEIVRIM.exe")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/build/CMakeFiles/vpn-gui.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE FILES
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-privacy-setup.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-harden.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-harden-revert.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-killswitch.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-anonymity.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-leaktest.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-privacy-addclient.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-setup.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-setup.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-monitor.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-monitor.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-status.ps1"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-control.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/vpn-full-setup.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/install.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/uninstall.bat"
    "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/README.md"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
  file(WRITE "D:/SOOBSHESTVA/VPN/VPN-TEIVRIM/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
