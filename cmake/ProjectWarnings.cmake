# Strict warning set. -Werror is intentionally NOT here yet; it is added in
# the closing pass-1 ticket (T12) once the codebase is clean.
function(project_apply_warnings TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor -Wold-style-cast
            -Wcast-align -Woverloaded-virtual -Wconversion
            -Wsign-conversion -Wnull-dereference -Wdouble-promotion
            -Wformat=2 -Wimplicit-fallthrough)
    endif()
endfunction()
