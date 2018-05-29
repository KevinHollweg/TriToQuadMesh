#include "gui.h"

using std::cout;
using std::endl;
using namespace std;


static void createBuffers(Mesh& mesh, std::vector<VertexNT>& vertices, std::vector<int>& indices, GLenum drawMode = GL_TRIANGLES)
{

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexNT), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );
    glEnableVertexAttribArray( 2 );

    glVertexAttribPointer(0,4, GL_FLOAT, GL_FALSE, sizeof(VertexNT), 0 );
    glVertexAttribPointer(1,4, GL_FLOAT, GL_FALSE, sizeof(VertexNT), (void*) (4 * sizeof(GLfloat)) );
    glVertexAttribPointer(2,2, GL_FLOAT, GL_FALSE, sizeof(VertexNT), (void*) (8 * sizeof(GLfloat)) );


    glGenBuffers(1, &mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

	mesh.numElements = (int)indices.size();
    mesh.drawMode = drawMode;
}

CG::CG(int w, int h) : Window(w,h)
{


    shaderManager.registerProgram("simple_color", SHADERTYPE_FLAG::VERTEX | SHADERTYPE_FLAG::FRAGMENT);
    shaderManager.update();

    {
        //load our planet mesh from file
        ObjLoader2 objLoader;
        objLoader.loadFile("data/icosphere.obj");
        std::vector<int> indices;
        for(auto f : objLoader.outTriangles)
        {
            indices.emplace_back(f.v[0]);indices.emplace_back(f.v[1]);indices.emplace_back(f.v[2]);
        }
        createBuffers(sphereMesh,objLoader.outVertices,indices,GL_TRIANGLES);
    }

    {
        //create ring mesh
        int segments = 100;
        float radius = 1;
        std::vector<int> indices;
        std::vector<VertexNT> vertices;
        for(int i = 0; i < segments; ++i)
        {
            float angle = float(i) / segments * glm::two_pi<float>();
            VertexNT v;
            v.position = vec4(radius*sin(angle),-cos(angle),0,1);
            vertices.push_back(v);
            indices.push_back(i);
        }
        indices.push_back(0);
        createBuffers(ringMesh,vertices,indices,GL_LINE_STRIP);
    }


    camera.lookAt( vec3(0,0,10), vec3(0), vec3(0,1,0));

}

void CG::update(float dt)
{
    //time += dt * timeScale;
	//
    //if(!ImGui::GetIO().WantCaptureMouse)
    //    camera.update(dt);
	//
    //// Use the following methods to create 4x4 transformation matrices:
    //// mat4 glm::translate(vec3 v);
    //// mat4 glm::scale(vec3 v);
    //// mat4 glm::rotate(float angle, vec3 axis);
	//
    //// a) Sun
    //sun = mat4(1); // <- Change this line
	//
    //// b) Earth
    //earth = glm::translate(vec3(earthOrbitRadius,0,0)); // <- Change this line
	//
    //// c) Moon
    //moon =  glm::translate(vec3(earthOrbitRadius + moonOrbitRadius,0,0)); // <- Change this line
	//
    //// d) Orbit Rings
    //earthOrbit = glm::scale(vec3(earthOrbitRadius));
    //moonOrbit = glm::translate(vec3(earthOrbitRadius + moonOrbitRadius,0,0)); // <- Change this line
	if (tested) {
		//cout << "button pressed" << endl;
	}
	else {
		//cout << "not pressed" << endl;
	}

}

void CG::render()
{
    //glClearColor(0,0,0,0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//
    //glEnable(GL_DEPTH_TEST);
	//
    ////wireframe mode
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	//
    //glUseProgram(shaderManager.getProgramGL("simple_color"));
	//
    //glm::mat4 m = camera.getProjectionMatrix() * camera.getViewMatrix();
    //glUniformMatrix4fv(0, 1, GL_FALSE, &m[0][0]);
	//
    ////sun
    //glUniform4fv(2,1,&vec4(1,1,0,1)[0]);
    //glUniformMatrix4fv(1, 1, GL_FALSE, &sun[0][0]);
    //sphereMesh.render();
	//
    ////earth
    //glUniform4fv(2,1,&vec4(0,0,1,1)[0]);
    //glUniformMatrix4fv(1, 1, GL_FALSE, &earth[0][0]);
    //sphereMesh.render();
    //glUniformMatrix4fv(1, 1, GL_FALSE, &earthOrbit[0][0]);
    //ringMesh.render();
	//
    ////moon
    //glUniform4fv(2,1,&vec4(0.6,0.6,0.6,1)[0]);
    //glUniformMatrix4fv(1, 1, GL_FALSE, &moon[0][0]);
    //sphereMesh.render();
    //glUniformMatrix4fv(1, 1, GL_FALSE, &moonOrbit[0][0]);
    //ringMesh.render();
	//
    //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

void CG::renderGui()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiSetCond_Once);
    ImGui::Begin("Solar System");

    //ImGui::SliderFloat("Time Scale",&timeScale,0,10);
	//
    //ImGui::SliderFloat("sunRotationTime",&sunRotationTime,1,100);
    //ImGui::SliderAngle("sunObliquity",&sunObliquity,-90,90);
    //ImGui::SliderFloat("sunRadius",&sunRadius,0,10);
	//
    //ImGui::SliderFloat("earthRotationTime",&earthRotationTime,0,5);
    //ImGui::SliderFloat("earthRevolutionTime",&earthRevolutionTime,0,500);
    //ImGui::SliderAngle("earthObliquity",&earthObliquity,-90,90);
    //ImGui::SliderFloat("earthRadius",&earthRadius,0,5);
    //ImGui::SliderFloat("earthOrbitRadius",&earthOrbitRadius,0,20);
	//
    //ImGui::SliderFloat("moonRevolutionTime",&moonRevolutionTime,0,50);
    //ImGui::SliderFloat("moonRotationTime",&moonRotationTime,0,50);
    //ImGui::SliderAngle("moonOrbitalInclination",&moonOrbitalInclination,-90,90);
    //ImGui::SliderAngle("moonObliquity",&moonObliquity,-90,90);
    //ImGui::SliderFloat("moonRadius",&moonRadius,0,2);
    //ImGui::SliderFloat("moonOrbitRadius",&moonOrbitRadius,0,6);
	ImGui::Checkbox("test", &tested);


    ImGui::End();
}
