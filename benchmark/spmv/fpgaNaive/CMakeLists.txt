set_source_files_properties(
    build/fpgaNaive_sim.o PROPERTIES
    EXTERNAL_OBJECT TRUE  # Identifies this as an object file
    GENERATED TRUE  # Avoids need for file to exist at configure-time
)

include_directories (build/fpgaNaive_MAIA_DFE_SIM/results)
add_executable(fpgaNaive_sim src/fpgaNaiveCpuCode.cpp build/fpgaNaive_sim.o)
target_link_libraries(fpgaNaive_sim maxeleros slic m pthread common ${MKL})

install (TARGETS fpgaNaive_sim DESTINATION ${INSTALL_TO_DIR})
