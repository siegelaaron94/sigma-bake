find_program(GLSLC_COMMAND glslc)
find_program(SPIRV_CROSS_COMMAND spirv-cross)

function(list_filter_extension LIST_VAR)
    list(GET ARGN 0 FLITER_REGEX)
    list(REMOVE_ITEM ARGN ${FLITER_REGEX})

    set(FLITER_REGEX ".*\\.${FLITER_REGEX}")
    foreach(ext ${ARGN})
        set(FLITER_REGEX "${FLITER_REGEX}|.*\\.${ext}")
    endforeach()

    set(TMP_LIST ${${LIST_VAR}})
    list(FILTER TMP_LIST INCLUDE REGEX "${FLITER_REGEX}")
    set(${LIST_VAR} ${TMP_LIST} PARENT_SCOPE)
endfunction()

function(add_package PACKAGE_NAME)
    set(options)
    set(oneValueArgs PACKAGE_ROOT)
    set(multiValueArgs)
    cmake_parse_arguments(add_package "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(PACKAGE_INCLUDES "-I${add_package_PACKAGE_ROOT}")

    # Package shaders
    set(SHADER_SOURCE_FILES "${add_package_UNPARSED_ARGUMENTS}")
    list_filter_extension(SHADER_SOURCE_FILES
        vert tesc tese geom frag
    )
	foreach(SHADER ${SHADER_SOURCE_FILES})
        get_filename_component(SHADER_NAME "${SHADER}" NAME_WE)
        get_filename_component(SHADER_DIRECTORY "${SHADER}" DIRECTORY)
        get_filename_component(SHADER_EXT "${SHADER}" EXT)

        if (NOT SHADER_DIRECTORY STREQUAL "")
            set(SHADER_DIRECTORY "${SHADER_DIRECTORY}/")
        endif()

        if(${SHADER_EXT} STREQUAL ".vert")
            set(SHADER_STAGE "SIGMA_ENGINE_VERTEX_SHADER")
            set(SHADER_TYPE "vertex")
        elseif(${SHADER_EXT} STREQUAL ".tesc")
            set(SHADER_STAGE "SIGMA_ENGINE_TESSELLATION_CONTROL_SHADER")
            set(SHADER_TYPE "tessellation_control")
        elseif(${SHADER_EXT} STREQUAL ".tese")
            set(SHADER_STAGE "SIGMA_ENGINE_TESSELLATION_EVALUATION_SHADER")
            set(SHADER_TYPE "tessellation_evaluation")
        elseif(${SHADER_EXT} STREQUAL ".geom")
            set(SHADER_STAGE "SIGMA_ENGINE_GEOMETRY_SHADER")
            set(SHADER_TYPE "geometry")
        elseif(${SHADER_EXT} STREQUAL ".frag")
            set(SHADER_STAGE "SIGMA_ENGINE_FRAGMENT_SHADER")
            set(SHADER_TYPE "fragment")
        endif()

        set(SHADER "${add_package_PACKAGE_ROOT}/${SHADER}")
        set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/data/shader/${SHADER_TYPE}/${SHADER_DIRECTORY}${SHADER_NAME}")
        get_filename_component(SHADER_OUTPUT_DIRECTORY "${SHADER_OUTPUT}" DIRECTORY)

        execute_process(
            COMMAND ${GLSLC_COMMAND} -D${SHADER_STAGE} ${PACKAGE_INCLUDES} -M ${SHADER}
            OUTPUT_VARIABLE R_DEPENDS
            WORKING_DIRECTORY ${add_package_PACKAGE_ROOT}
        )
        string(REPLACE "${SHADER_NAME}${SHADER_EXT}.spv: " "" R_DEPENDS ${R_DEPENDS})
        string(REPLACE " " ";" R_DEPENDS ${R_DEPENDS})

        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}${SHADER_EXT}_spv"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIRECTORY}
            COMMAND ${GLSLC_COMMAND} --target-env=opengl -D${SHADER_STAGE} ${PACKAGE_INCLUDES} "${SHADER}" -o "${SHADER_OUTPUT}${SHADER_EXT}_spv"
            DEPENDS ${R_DEPENDS}
        )

        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}${SHADER_EXT}_spv.json"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIRECTORY}
            COMMAND ${SPIRV_CROSS_COMMAND} "${SHADER_OUTPUT}${SHADER_EXT}_spv" --reflect --output "${SHADER_OUTPUT}${SHADER_EXT}_spv.json"
            DEPENDS "${SHADER_OUTPUT}${SHADER_EXT}_spv"
        )

        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}"
            COMMAND sigma-bake -o "${CMAKE_BINARY_DIR}" "${SHADER_OUTPUT}${SHADER_EXT}_spv"
            DEPENDS "${SHADER_OUTPUT}${SHADER_EXT}_spv" "${SHADER_OUTPUT}${SHADER_EXT}_spv.json"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/data/shader"
        )

        list(APPEND SHADER_OUTPUTS "${SHADER_OUTPUT}")
    endforeach()

    # Package textures
    set(TEXTURE_SOURCE_FILES "${add_package_UNPARSED_ARGUMENTS}")
    list_filter_extension(TEXTURE_SOURCE_FILES
        tiff tif jpg jpeg jpe jif jfif jfi png hdr
    )
    foreach(TEXTURE ${TEXTURE_SOURCE_FILES})
        get_filename_component(TEXTURE_NAME "${TEXTURE}" NAME_WE)
        get_filename_component(TEXTURE_DIRECTORY "${TEXTURE}" DIRECTORY)
        get_filename_component(TEXTURE_EXT "${TEXTURE}" EXT)

        if (NOT TEXTURE_DIRECTORY STREQUAL "")
            set(TEXTURE_DIRECTORY "${TEXTURE_DIRECTORY}/")
        endif()

        set(TEXTURE "${add_package_PACKAGE_ROOT}/${TEXTURE}")
        set(TEXTURE_SETTINGS "${RESOURCE_PACKAGE_DIRECTORY}/${TEXTURE_DIRECTORY}${TEXTURE_NAME}.stex")
        set(TEXTURE_OUTPUT "${CMAKE_BINARY_DIR}/data/texture/${TEXTURE_DIRECTORY}${TEXTURE_NAME}")

        set(TEXTURE_DEPENDS "${TEXTURE}")
        if(EXISTS "${TEXTURE_SETTINGS}")
            set(TEXTURE_DEPENDS "${TEXTURE}" "${TEXTURE_SETTINGS}")
        endif()

        add_custom_command(
            OUTPUT ${TEXTURE_OUTPUT}
            COMMAND sigma-bake -o "${CMAKE_BINARY_DIR}" "${TEXTURE}"
            DEPENDS ${TEXTURE_DEPENDS}
            WORKING_DIRECTORY ${add_package_PACKAGE_ROOT}
        )

        list(APPEND TEXTURE_OUTPUTS ${TEXTURE_OUTPUT})
    endforeach()

    # Package materials
    set(MATERIAL_SOURCE_FILES "${add_package_UNPARSED_ARGUMENTS}")
    list_filter_extension(MATERIAL_SOURCE_FILES
        smat
    )
    foreach(MATERIAL ${MATERIAL_SOURCE_FILES})
        get_filename_component(MATERIAL_NAME "${MATERIAL}" NAME_WE)
        get_filename_component(MATERIAL_DIRECTORY "${MATERIAL}" DIRECTORY)
        get_filename_component(MATERIAL_EXT "${MATERIAL}" EXT)

        if (NOT MATERIAL_DIRECTORY STREQUAL "")
            set(MATERIAL_DIRECTORY "${MATERIAL_DIRECTORY}/")
        endif()

        set(MATERIAL "${add_package_PACKAGE_ROOT}/${MATERIAL}")
        set(MATERIAL_OUTPUT "${CMAKE_BINARY_DIR}/data/material/${MATERIAL_DIRECTORY}${MATERIAL_NAME}")

        # TODO: make this smarter
        set(MATERIAL_DEPENDS "${MATERIAL}" ${SHADER_OUTPUTS} ${TEXTURE_OUTPUTS})

        add_custom_command(
            OUTPUT ${MATERIAL_OUTPUT}
            COMMAND sigma-bake -o "${CMAKE_BINARY_DIR}" "${MATERIAL}"
            DEPENDS ${MATERIAL_DEPENDS}
            WORKING_DIRECTORY ${add_package_PACKAGE_ROOT}
        )

        list(APPEND MATERIAL_OUTPUTS ${MATERIAL_OUTPUT})
    endforeach()

    #Package static meshes
    set(STATIC_MESH_SOURCE_FILES "${add_package_UNPARSED_ARGUMENTS}")
    list_filter_extension(STATIC_MESH_SOURCE_FILES
        3ds dae fbx ifc-step ase dxf hmp md2 md3 md5 mdc mdl nff ply stl x obj
        opengex smd lwo lxo lws ter ac3d ms3d cob q3bsp xgl csm bvh b3d ndo q3d
        gltf 3mf blend
    )
    foreach(STATIC_MESH ${STATIC_MESH_SOURCE_FILES})
        get_filename_component(STATIC_MESH_NAME "${STATIC_MESH}" NAME_WE)
        get_filename_component(STATIC_MESH_DIRECTORY "${STATIC_MESH}" DIRECTORY)
        get_filename_component(STATIC_MESH_EXT "${STATIC_MESH}" EXT)

        if (NOT STATIC_MESH_DIRECTORY STREQUAL "")
            set(STATIC_MESH_DIRECTORY "${STATIC_MESH_DIRECTORY}/")
        endif()

        set(STATIC_MESH "${add_package_PACKAGE_ROOT}/${STATIC_MESH}")
        set(STATIC_MESH_OUTPUT "${CMAKE_BINARY_DIR}/data/static_mesh/${STATIC_MESH_DIRECTORY}${STATIC_MESH_NAME}")

        # TODO: make this smarter
        set(STATIC_MESH_DEPENDS "${STATIC_MESH}" ${SHADER_OUTPUTS} ${TEXTURE_OUTPUTS} ${MATERIAL_OUTPUTS})

        add_custom_command(
            OUTPUT ${STATIC_MESH_OUTPUT}
            COMMAND sigma-bake -o "${CMAKE_BINARY_DIR}" "${STATIC_MESH}"
            DEPENDS ${STATIC_MESH_DEPENDS}
            WORKING_DIRECTORY ${add_package_PACKAGE_ROOT}
        )

        list(APPEND STATIC_MESH_OUTPUTS ${STATIC_MESH_OUTPUT})
    endforeach()

   add_custom_target(${PACKAGE_NAME} DEPENDS ${SHADER_OUTPUTS} ${TEXTURE_OUTPUTS} ${MATERIAL_OUTPUTS} ${STATIC_MESH_OUTPUTS})
endfunction()
