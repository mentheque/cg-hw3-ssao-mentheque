set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/model.vert
    Shaders/coneScaling.vert
    Shaders/pointBlowout.vert
    Shaders/gbuffModel.frag
    Shaders/gbuffShineThrough.frag
    Shaders/ssao.frag
    Shaders/ssLighting.frag
    Shaders/ssLighting.vert
    Shaders/ssOcclusionOrDiffuse.frag
    Shaders/ssSimple.vert
    Shaders/ssSimpleBlur.frag
    resources.qrc
 "FpvCamera.h" "FpvCamera.cpp" "KeyPressContainer.h" "KeyPressContainer.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp"
"ColorButton.h" "ColorButton.cpp" "utils.h" "utils.cpp" "Values.h" "ScreenspacePipeline.cpp" "ScreenspacePipeline.h" "ssaoUniforms.h" "ssaoUniforms.cpp")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(ssao ${SRCS} "FpvCamera.h" "FpvCamera.cpp" "Model.h" "Model.cpp" "Lights.h" "Lights.cpp" "projectionPointSearch.h" "projectionPointSearch.cpp"  "Values.h" "ScreenspacePipeline.cpp")

target_link_libraries(ssao
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)