
--vertex
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout (location = 0) uniform mat4 projView;
layout (location = 1) uniform mat4 model;
 
out vec3 normal;

void main() {
        gl_Position = projView * model * vec4(in_position, 1);
        normal = in_normal;
}

--fragment

layout (location = 0) out vec4 out0;

layout (location = 2) uniform vec4 color;

in vec3 normal;

void main() {

    out0 = color;
//    const vec3 lightDir = normalize(vec3(1,2,1));

//    float diff = max(dot(lightDir,normal),0);
//        out0 = diff * color;
}
