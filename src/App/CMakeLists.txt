set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/model.frag
    Shaders/model.vert
    Shaders/coneScaling.vert
    Shaders/pointBlowout.vert
    Shaders/shineThrough.frag
    resources.qrc
 "FpvCamera.h" "FpvCamera.cpp" "KeyPressContainer.h" "KeyPressContainer.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp"
"ColorButton.h" "ColorButton.cpp" "utils.h" "utils.cpp" "Values.h")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(morph ${SRCS} "FpvCamera.h" "FpvCamera.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp"  "Values.h")

target_link_libraries(morph
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)