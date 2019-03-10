# Kinect SDK is Windows only
if (WIN32 AND opentrack-intel)
	# Setup cache variable to Kinect SDK path
	set(SDK_KINECT20 "$ENV{KINECTSDK20_DIR}" CACHE PATH "Kinect SDK path")
endif()

# I'm guessing if we make this a function it's going to cause all sorts of problems and will need to be fixed in many ways
macro(otr_kinect_setup)
    # If we have a valid SDK path, try build that tracker
    if(SDK_KINECT20)
        if(MSVC)
        # workaround warning in SDK
            target_compile_options(${self} PRIVATE "-wd4471")
        endif()

        # Add include path to Kinect SDK
        target_include_directories(${self} SYSTEM PRIVATE "${SDK_KINECT20}/inc")

        # Check processor architecture
        if(opentrack-64bit)
            # 64 bits
            set(kinect-arch-dir "x64")
        else()
            # 32 bits
            set(kinect-arch-dir "x86")
        endif()

        # Link against Kinect SDK libraries
        target_link_libraries(${self} "${SDK_KINECT20}/lib/${kinect-arch-dir}/Kinect20.lib" "${SDK_KINECT20}/lib/${kinect-arch-dir}/Kinect20.Face.lib")
        # Link against video utilities, needed for video preview
        #target_link_libraries(${self} opentrack-video)

        # Install Kinect Face DLL
        install(FILES "${SDK_KINECT20}/Redist/Face/${kinect-arch-dir}/Kinect20.Face.dll" DESTINATION "${opentrack-hier-pfx}" PERMISSIONS ${opentrack-perms-exec})
        # Install Kinect Face Database
        install(DIRECTORY "${SDK_KINECT20}/Redist/Face/${kinect-arch-dir}/NuiDatabase" DESTINATION "${opentrack-hier-pfx}")

        set(redist-dir "${CMAKE_SOURCE_DIR}/redist/${kinect-arch-dir}")
        install(
            FILES "${redist-dir}/msvcp110.dll" "${redist-dir}/msvcr110.dll"
            DESTINATION "${opentrack-hier-pfx}"
            PERMISSIONS ${opentrack-perms-exec}
        )
    endif()
endmacro()
