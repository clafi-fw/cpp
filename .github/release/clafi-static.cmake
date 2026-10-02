# The toolchain of ClaFi's Linux releases: clang over libc++, with the C++ runtime linked into the
# executable rather than required of the machine it runs on. The Containerfile beside this file
# states CLAFI_LLVM_ROOT.

set(CLAFI_LLVM_ROOT "$ENV{CLAFI_LLVM_ROOT}" CACHE PATH "The LLVM installation to build against")

set(CMAKE_C_COMPILER "${CLAFI_LLVM_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER "${CLAFI_LLVM_ROOT}/bin/clang++")

# Here and not on a target: CMake builds the synthesized std module from CMAKE_CXX_FLAGS alone,
# and clang refuses a BMI whose standard library differs from the consumer's.
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")

# A module manifest names its sources relative to itself, and a distribution may ship copies at
# depths where those paths do not resolve. Each candidate is tried, and the first whose std
# module source is on disk wins.
execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libc++.modules.json
    OUTPUT_VARIABLE clafi_json_from_clang
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

set(clafi_json_candidates
    "${CLAFI_LLVM_ROOT}/lib/libc++.modules.json"
    "${clafi_json_from_clang}"
    "${CLAFI_LLVM_ROOT}/share/libc++/v1/libc++.modules.json")
file(GLOB_RECURSE clafi_json_swept "${CLAFI_LLVM_ROOT}/*/libc++.modules.json")
list(APPEND clafi_json_candidates ${clafi_json_swept})

set(clafi_modules_json "")
set(clafi_json_tried "")
foreach(candidate IN LISTS clafi_json_candidates)
    if(NOT candidate OR NOT EXISTS "${candidate}")
        continue()
    endif()
    get_filename_component(candidate "${candidate}" ABSOLUTE)
    get_filename_component(clafi_json_dir "${candidate}" DIRECTORY)
    file(READ "${candidate}" clafi_json_text)
    string(JSON clafi_std_source ERROR_VARIABLE clafi_json_error
           GET "${clafi_json_text}" modules 0 source-path)
    if(clafi_json_error)
        continue()
    endif()
    if(NOT IS_ABSOLUTE "${clafi_std_source}")
        set(clafi_std_source "${clafi_json_dir}/${clafi_std_source}")
    endif()
    get_filename_component(clafi_std_source "${clafi_std_source}" ABSOLUTE)
    if(EXISTS "${clafi_std_source}")
        set(clafi_modules_json "${candidate}")
        break()
    endif()
    string(APPEND clafi_json_tried "\n    ${candidate}\n      names ${clafi_std_source}, which is not there")
endforeach()

if(NOT clafi_modules_json)
    message(FATAL_ERROR
            "No usable libc++.modules.json under ${CLAFI_LLVM_ROOT}. Without one `import std` "
            "has nothing to build from.${clafi_json_tried}\n"
            "Install libc++-<version>-dev, or build libc++ with -DLIBCXX_INSTALL_MODULES=ON.")
endif()

# Derived, not a knob. FORCE keeps a build tree configured with a wrong path from holding on to it.
set(CMAKE_CXX_STDLIB_MODULES_JSON "${clafi_modules_json}"
    CACHE FILEPATH "libc++'s module manifest" FORCE)

# -nostdlib++ drops the automatic -lc++, which resolves to the shared object. --exclude-libs,ALL
# keeps the embedded libc++ out of the dynamic symbol table, so it cannot meet another copy.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib++ -Wl,--exclude-libs,ALL")

# The archives go at the end of the link line, after the objects that need them. An archive in
# CMAKE_EXE_LINKER_FLAGS stands before the objects and contributes nothing. The LLVM root is asked
# first: a system copy may belong to another LLVM than the headers being compiled against.
set(clafi_cxx_archives "")
foreach(archive IN ITEMS libc++.a libc++abi.a)
    set(clafi_archive_path "${CLAFI_LLVM_ROOT}/lib/${archive}")
    if(NOT EXISTS "${clafi_archive_path}")
        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${archive}"
            OUTPUT_VARIABLE clafi_archive_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        get_filename_component(clafi_archive_path "${clafi_archive_path}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${clafi_archive_path}")
        message(FATAL_ERROR
                "${archive} was not found, so the C++ runtime cannot be linked in. "
                "Install libc++-<version>-dev.")
    endif()
    string(APPEND clafi_cxx_archives " ${clafi_archive_path}")
endforeach()

set(CMAKE_CXX_STANDARD_LIBRARIES "${clafi_cxx_archives}")

# The unwinder stays shared. libgcc_s.so.1 answers what libc++abi.a leaves undefined, it is on
# every glibc system, and a process has one unwinder.
