cmake_minimum_required(VERSION 3.12)

project(CG_LoaderCubo)

# Adiciona o executável do projeto
add_executable(CG_LoaderCubo "main.cpp")

# Configuração do CMake para encontrar o GLM, o GLFW, o GLEW, o STB
target_include_directories(CG_LoaderCubo PRIVATE deps/glm 
                                        deps/glfw/include
                                        deps/glew/include
                                        deps/stb
                                        deps/assimp/include
                                     
)
                                       

target_link_directories(CG_LoaderCubo PRIVATE deps/glfw/lib-vc2019
                                              deps/glew/lib/Release/x64
                                              deps/assimp/lib
                                        
                  
)

# Liga o GLFW, o GLEW e o OPENGL ao executável
target_link_libraries(CG_LoaderCubo PRIVATE glfw3.lib glew32.lib assimp-vc143-mt.lib opengl32.lib)

add_custom_command(TARGET CG_LoaderCubo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   "${CMAKE_SOURCE_DIR}/deps/glew/bin/Release/x64/glew32.dll" 
                   "${CMAKE_BINARY_DIR}/glew32.dll")


                   
add_custom_command(TARGET CG_LoaderCubo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${CMAKE_SOURCE_DIR}/deps/assimp/bin"
                   "${CMAKE_BINARY_DIR}")