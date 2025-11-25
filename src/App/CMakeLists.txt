set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/diffuse.frag
    Shaders/diffuse.vert
    Textures/voronoi.png

    resources.qrc
 "FpvCamera.h" "FpvCamera.cpp")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(demo-app ${SRCS} "FpvCamera.h" "FpvCamera.cpp")

target_link_libraries(demo-app
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)