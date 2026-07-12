cmake_minimum_required(VERSION 3.20)
project(VPN-TEIVRIM VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vpn-gui WIN32 vpn-gui.cpp)

target_link_libraries(vpn-gui PRIVATE
    comctl32
    shell32
    user32
    kernel32
    mingw32
)

target_compile_options(vpn-gui PRIVATE
    -municode
    -O2
    -Wall
)

target_link_options(vpn-gui PRIVATE
    -municode
    -static
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
