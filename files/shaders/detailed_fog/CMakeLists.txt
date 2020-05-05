if (NOT DEFINED OPENMW_SHADERS_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/shaders/detailed_fog)

set(SHADER_FILES
    ../shadowcasting_vertex.glsl
    ../shadowcasting_fragment.glsl
    water_vertex.glsl
    water_fragment.glsl
    objects_vertex.glsl
    objects_fragment.glsl
    terrain_vertex.glsl
    terrain_fragment.glsl
    lighting.glsl
    parallax.glsl
    fog.glsl
    fog_simple.glsl
    atmosphere_vertex.glsl
    atmosphere_fragment.glsl
    atmosphere_night_vertex.glsl
    atmosphere_night_fragment.glsl
    clouds_vertex.glsl
    clouds_fragment.glsl
    sunflash_fragment.glsl
    sunflash_vertex.glsl
    sun_vertex.glsl
    sun_fragment.glsl
    moon_vertex.glsl
    moon_fragment.glsl
    world.glsl
    sunlight.glsl
    sunlight_calculation.glsl
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_SHADERS_ROOT} ${DDIRRELATIVE} "${SHADER_FILES}")
