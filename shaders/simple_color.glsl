
--vertex
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout (location = 0) uniform mat4 projView;
layout (location = 1) uniform mat4 model;
 
void main() {
        gl_Position = projView * model * vec4(in_position, 1);
}

--fragment

layout (location = 2) uniform vec4 color;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = color;
}
