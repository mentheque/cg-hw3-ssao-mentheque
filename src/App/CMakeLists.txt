set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/diffuse.frag
    Shaders/diffuse.vert
    Shaders/model.frag
    Shaders/model.vert
    Textures/voronoi.png
    resources.qrc
 "FpvCamera.h" "FpvCamera.cpp" "KeyPressContainer.h" "KeyPressContainer.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(demo-app ${SRCS} "FpvCamera.h" "FpvCamera.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp" )

target_link_libraries(demo-app
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)