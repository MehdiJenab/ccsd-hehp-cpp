# Strict warning set with -Werror. -Wold-style-cast and -Wcast-function-type
# are dropped because they fire exclusively inside OpenMPI macros
# (MPI_DOUBLE et al.) which we cannot edit.
function(project_apply_warnings TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Werror -Wpedantic
            -Wshadow
            -Wcast-align -Woverloaded-virtual -Wconversion
            -Wsign-conversion -Wnull-dereference -Wdouble-promotion
            -Wformat=2 -Wimplicit-fallthrough
            -Wno-cast-function-type -Wno-old-style-cast
            -Wno-non-virtual-dtor)
    endif()
endfunction()
