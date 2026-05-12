build_dir := "build"
c_compiler := "clang"
cxx_compiler := "clang++"
cxx_standard := "23"

# Clean build artifacts
clean:
    rm -f compile_commands.json
    rm -rf {{ build_dir }}

# Configure the project
configure:
    cmake -S . \
        -B {{ build_dir }} \
        -D CMAKE_C_COMPILER="{{ c_compiler }}" \
        -D CMAKE_CXX_COMPILER="{{ cxx_compiler }}" \
        -D CMAKE_CXX_STANDARD={{ cxx_standard }} \
        -D CMAKE_CXX_STANDARD_REQUIRED=ON \
        -D CMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -D CMAKE_BUILD_TYPE=Debug \
        -G Ninja

    ln -sf {{ build_dir }}/compile_commands.json .

# Build the server
build target="all": configure
    cmake --build {{ build_dir }} --target {{ target }}

# Run the server
[working-directory('bin')]
run target="OpenGraal": build
    ../{{ build_dir }}/{{ target }}

# Rebuild the server from scratch
rebuild: clean build
