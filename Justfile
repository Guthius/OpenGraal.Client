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

# Build everything, or a single target
build target="all": configure
    cmake --build {{ build_dir }} --target {{ target }}

# Run the client
[working-directory('bin')]
run-client: (build "OpenGraal")
    ../{{ build_dir }}/OpenGraal

# Run the server against a gameworld
[working-directory('bin')]
run-server world="world" port="14900": (build "OpenGraalServer")
    ../{{ build_dir }}/OpenGraalServer {{ world }} --port={{ port }}

# Run a script through the standalone GS2 interpreter
run-script script: (build "gs2")
    ./{{ build_dir }}/gs2 {{ script }}

# Run the test suite
test filter="": (build "opengraal-tests")
    ./{{ build_dir }}/opengraal-tests {{ filter }}

# Rebuild from scratch
rebuild: clean build

# Format all source files that are part of the project
format:
    find ./src ./tests ./tools -type f \( -name '*.cpp' -o -name '*.hpp' \) \
        -print0 | xargs -0 clang-format -i

# Report clang-tidy findings
lint: configure
    find ./src ./tools -type f -name '*.cpp' \
        -print0 | xargs -0 clang-tidy -p {{ build_dir }} --quiet
