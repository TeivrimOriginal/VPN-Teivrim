cmake_minimum_required(VERSION 3.20)
project(VPN-TEIVRIM VERSION 2.4.2 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

add_executable(vpn-gui WIN32 vpn-gui.cpp qrcodegen.cpp manifest.rc
    src/exec.cpp
    src/config.cpp
    src/peers.cpp
    src/killswitch.cpp
    src/privacy.cpp
    src/wizard.cpp
)

if(BUILD_TESTING)
    add_test(NAME vpn-gui-self-test COMMAND $<TARGET_FILE:vpn-gui> --self-test)
endif()

target_include_directories(vpn-gui PRIVATE src)

target_link_libraries(vpn-gui PRIVATE
    comctl32
    shell32
    user32
    kernel32
    mingw32
    crypt32
    iphlpapi
)

target_compile_options(vpn-gui PRIVATE
    -municode
    -O2
    -Wall
)

target_link_options(vpn-gui PRIVATE
    -municode
    -static
    -s
)

set_target_properties(vpn-gui PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "VPN-TEIVRIM"
)

install(TARGETS vpn-gui DESTINATION bin)
install(FILES
    vpn-privacy-setup.ps1
    vpn-harden.ps1
    vpn-harden-revert.ps1
    vpn-killswitch.ps1
    vpn-anonymity.ps1
    vpn-leaktest.ps1
    vpn-privacy-addclient.ps1
    vpn-setup.ps1
    vpn-setup.bat
    vpn-monitor.ps1
    vpn-monitor.bat
    vpn-status.ps1
    vpn-control.bat
    vpn-full-setup.bat
    install.bat
    uninstall.bat
    README.md
    DESTINATION bin
)
