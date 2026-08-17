#[[
    Warning: This module is private, may be modified or removed in the future, please use with caution.
]] #

include_guard(DIRECTORY)

#[[
    Create the names of output files preserving relative dirs. (Ported from MOC command)

    qm_make_output_file(<infile> <prefix> <ext> <OUT>)

    OUT: output source file paths
#]]
function(qm_make_output_file _infile _prefix _ext _out)
    string(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
    string(LENGTH ${_infile} _infileLength)
    set(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})

    if(_infileLength GREATER _binlength)
        string(SUBSTRING "${_infile}" 0 ${_binlength} _checkinfile)

        if(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
            file(RELATIVE_PATH _name ${CMAKE_CURRENT_BINARY_DIR} ${_infile})
        else()
            file(RELATIVE_PATH _name ${CMAKE_CURRENT_SOURCE_DIR} ${_infile})
        endif()
    else()
        file(RELATIVE_PATH _name ${CMAKE_CURRENT_SOURCE_DIR} ${_infile})
    endif()

    if(CMAKE_HOST_WIN32 AND _name MATCHES "^([a-zA-Z]):(.*)$") # absolute path
        set(_name "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    endif()

    set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${_name}")
    string(REPLACE ".." "__" _outfile ${_outfile})
    get_filename_component(_outpath ${_outfile} PATH)
    get_filename_component(_outfile ${_outfile} NAME_WLE)

    file(MAKE_DIRECTORY ${_outpath})
    set(${_out} ${_outpath}/${_prefix}${_outfile}.${_ext} PARENT_SCOPE)
endfunction()

#[[
    Create a custom command to run `xxd`.

    qm_add_binary_resource(<input> <output>)
#]]
function(qm_add_binary_resource _input _output)
    find_program(_xxd "xxd")
    if(NOT _xxd)
        get_filename_component(_name ${_output} NAME)
        string(MAKE_C_IDENTIFIER ${_name} _name)
        set(_cmd "${CMAKE_COMMAND}"
            -D "input=${_input}"
            -D "output=${_output}"
            -D "name=${_name}"
            -P "${QMSETUP_MODULES_DIR}/scripts/xxd.cmake")
    else()
        set(_cmd "${_xxd}" "-i" "${_input}" "${_output}")
    endif()

    add_custom_command(
        OUTPUT ${_output}
        COMMAND ${_cmd}
        DEPENDS ${_input}
    )
endfunction()
