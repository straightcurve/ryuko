# ryuko

`ryuko`(流光, meaning "flowing light") is a transpiler from a custom shader language to GLSL.

## Features

1. Shader Parsing

   Parses GLSL code to extract functions and varyings.

2. Transformation

   Modifies main shader functions' return values, ensuring compatibility with Vulkan requirements:
    - Vertex shader: `vert()`'s return expression is converted into assignment to `gl_Position`.
    - Fragment shader: `frag()`'s return expression is assigned to a custom `out` variable.

3. GLSL Code Emission

   Transpiles and emits modified vertex and fragment shader code to separate GLSL files.

4. Error Handling

   Provides detailed error messages with formatted outputs for easier debugging.

## Getting Started

### Prerequisites

- C++ Compiler: Requires a compiler that supports C++20.
- Libraries:
    - fmt: For formatted text output.
- CMake: Building the project.
- [mei](https://github.com/straightcurve/mei): Building the project.

⚠️ Requires the `fmt` [xrepo](https://xrepo.xmake.io/) package to be installed. It can be installed with:

```bash
xrepo install -y fmt
```

## Compilation

### Clone the repository:

```bash
git clone https://github.com/straightcurve/ryuko.git
cd ryuko
```

### Generate CMakeLists.txt

```bash
npx @sweetacid/mei
```

### Create a build directory:

```bash
mkdir build && cd build
```

### Configure and build:

```bash
cmake ..
make
```

## Code Structure

### Parser

Extracts functions, varyings and shader version.

### Transpiler

Modifies shader code to align with Vulkan requirements:

- Rewrites `return` statements into appropriate assignments.
- Adds varying variable declarations when necessary.

### Emitter

Responsible for emitting the final GLSL code:

- Ensures functions are emitted in dependency order.
- Handles GLSL version declarations and varying declarations.

### Main Functionality (`transpile`)

Orchestrates the parsing, transpiling and emission of shaders.

## Example Input/Output

### Input Shader:

```glsl
#version 450

varying vec4 Position;
varying vec4 Color;

vec4 vert() {
    Color = vec4(Position.zyx, 1.0);

    return vec4(Position.xyz, 1.0);
}

vec4 frag() {
    return Color;
}
```

### Output Vertex Shader:

```glsl
#version 450

layout (location = 0) in vec4 Position;
layout (location = 0) out vec4 Color;

void main() {
    Color = vec4(Position.zyx, 1.0);
    gl_Position = vec4(Position.xyz, 1.0);
}
```

### Output Fragment Shader:

```glsl
#version 450

layout (location = 0) in vec4 Color;
layout (location = 0) out vec4 ryuko_outColor;

void main() {
    ryuko_outColor = Color;
}
```

## Error Handling

Reports errors such as:

- Missing vertex or fragment main functions.
- Invalid GLSL syntax __(partial)__.
- Parsing failures due to unsupported constructs.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests for bug fixes, enhancements, or additional
features.