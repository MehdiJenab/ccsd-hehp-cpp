# Kokkos mdspan reference impl (single-header) via FetchContent.
include(FetchContent)
FetchContent_Declare(
    kokkos_mdspan
    GIT_REPOSITORY https://github.com/kokkos/mdspan.git
    GIT_TAG        9ceface91483775a6c74d06ebf717bbb2768452f  # 2024-08
)
FetchContent_MakeAvailable(kokkos_mdspan)

# Treat mdspan headers as SYSTEM so project -Werror flags (e.g. -Wsign-conversion)
# do not trip on the third-party reference implementation.
get_target_property(_ccsd_mdspan_inc mdspan INTERFACE_INCLUDE_DIRECTORIES)
if(_ccsd_mdspan_inc)
    set_target_properties(mdspan PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_ccsd_mdspan_inc}")
endif()

add_library(ccsd_mdspan INTERFACE)
target_link_libraries(ccsd_mdspan INTERFACE std::mdspan)
add_library(ccsd::mdspan ALIAS ccsd_mdspan)
