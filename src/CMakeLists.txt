set(PARETTO_CORE_SOURCE_FILES
    common/AudioMath.h
    common/Constants.h
    common/Denormals.h
    common/DspContext.h
    common/EnvFollower.h
    common/Lfo.h
    common/Ramp32.h
    common/Rng.h
    common/Smoothers.h
    common/SmoothRandom.h
    common/SoftClipper.h
    common/StepSequencer.h
    common/Svf.h
    common/DelayLine.h
    gate/GateEngine.cpp
    gate/GateEngine.h
    gate/GateParameters.h
    motion/MotionEngine.cpp
    motion/MotionEngine.h
    motion/MotionParameters.h
)

add_library(ParettoCore STATIC ${PARETTO_CORE_SOURCE_FILES})

target_include_directories(ParettoCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_source_files_properties(${PARETTO_CORE_SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")
