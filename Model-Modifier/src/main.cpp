#include <iostream>
#include <ctime>
#include <unordered_map>

#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_glfw.h"
#include "external/imgui/imgui_impl_opengl3.h"
#include "external/stb/stb_image_write.h"

#include "renderer/Window.h"
#include "renderer/Input.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/IndexBuffer.h"
#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "scene/object/Object.h"
#include "scene/object/Objects.h"
#include "scene/Mesh.h"
#include "scene/Material.h"
#include "scene/Light.h"
#include "scene/surface/Surface.h"

enum shader
{
    GOURAND,
    NORMAL,
    PHONG,
    BLINNPHONG,
    GOOCH,
    CEL,
};

enum renderMode
{
    POLYGON,
    WIREFRAME,
    POINTCLOUD
};

void saveImage(const char* filepath, GLFWwindow* window) 
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    GLsizei stride = 3 * width;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    GLsizei bufferSize = stride * height;
    std::vector<char> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filepath, width, height, 3, buffer.data(), stride);
}

int main()
{
    unsigned int screenWidth = 1440;
    unsigned int screenHeight = 810;
    float aspectRatio = (float)screenWidth / screenHeight;
    Window window(screenWidth, screenHeight, "Model Modifier", NULL);

    // build object from obj file
    Objects objects;
    int currObject = BUNNY;
    int nextObject;
    Object obj = objects.findObj(currObject);
    Material meshMat;

    VertexBufferLayout layout;
    layout.Push<float>(3); // 3d coordinates
    layout.Push<float>(3); // normals

    // render mode
    int currRenderMode = POLYGON;
    int nextRenderMode;
    glPointSize(2);

    // shading type
    int currShadingType = FLAT;
    int nextShadingType;

    Mesh mesh(obj, currShadingType);
    // keep track of number of faces
    int numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size());

    // build openGL objects using mesh
    VertexArray objectVA;
    VertexBuffer objectVB(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
    // bind vertex buffer to vertex array
    objectVA.AddBuffer(objectVB, layout);
    IndexBuffer objectIB(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);

    // shaders
    std::string phongVertexPath = "res/shaders/phong.vert";
    std::string phongFragmentPath = "res/shaders/phong.frag";
    Shader phongShader(phongVertexPath, phongFragmentPath);

    std::string blinnPhongFragmentPath = "res/shaders/blinnPhong.frag";
    Shader blinnPhongShader(phongVertexPath, blinnPhongFragmentPath);

    std::string normalVertexPath = "res/shaders/normal.vert";
    std::string normalFragmentPath = "res/shaders/normal.frag";
    Shader normalShader(normalVertexPath, normalFragmentPath);

    std::string gourandVertexPath = "res/shaders/gourand.vert";
    std::string gourandFragmentPath = "res/shaders/gourand.frag";
    Shader gourandShader(gourandVertexPath, gourandFragmentPath);

    std::string goochFragmentPath = "res/shaders/gooch.frag";
    Shader goochShader(phongVertexPath, goochFragmentPath);

    std::string celFragmentPath = "res/shaders/cel.frag";
    Shader celShader(phongVertexPath, celFragmentPath);

    int currShader = NORMAL;
    int nextShader;
    Shader shader = normalShader;

    // camera setup
    float yaw = 1.5f; // radians
    float pitch = 0.3333f; // radians
    float radius = 3.0f;
    Camera camera(pitch, yaw, radius);
    camera.ResetView();

    float rotationAngle = 0.0f; // rotation angle of the object mesh
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projMatrix = glm::perspective(glm::radians(camera.m_FOV), aspectRatio, 0.1f, 1000.0f);

    // lighting
    Light light = Light();
    int* toggled = new int[3];

    // Gooch variables
    float* gooch_warm = new float[3] {1, 0.25, 0};
    float* gooch_cool = new float[3] {0, 0.75, 1};
    float gooch_alpha = 0.2f;
    float gooch_beta = 0.2f;

    // upload uniforms
    shader.Bind();
    shader.SetUniformMat4f("u_Model", modelMatrix);
    shader.SetUniformMat4f("u_View", camera.GetViewMatrix());
    shader.SetUniformMat4f("u_Projection", projMatrix);
    if (currShader == GOURAND || currShader == PHONG || currShader == BLINNPHONG || currShader == GOOCH || currShader == CEL)
    {
        shader.SetUniform3fv("light_pos", 3, light.m_Pos);
        shader.SetUniform3fv("light_col", 3, light.m_Col);

        shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
        shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
        shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
        shader.SetUniform1f("shine", meshMat.m_Shine);
    }
    if (currShader == GOOCH)
    {
        shader.SetUniform3fv("warm", 1, gooch_warm);
        shader.SetUniform3fv("cool", 1, gooch_cool);
        shader.SetUniform1f("alpha", gooch_alpha);
        shader.SetUniform1f("beta", gooch_beta);
    }

    // openGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // framerate mode
    bool framerate = false;
    // num of triangles mode
    bool triangles = false;

    GLFWwindow* windowID = window.GetID();
    // input initialization & input callbacks
    Input::Init(windowID);

    // keyboard movement variables
    float currentTime = 0.0f;
    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    // mouse movement variables
    double currXpos, currYpos, deltaX, deltaY;
    double lastXpos = 0.0;
    double lastYpos = 0.0;
    float sens = 200.0f;

    // Setup Dear ImGui context
    ImGui::CreateContext();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(windowID, true);
    ImGui_ImplOpenGL3_Init();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(windowID))
    {
        // reset object and shader per frame
        nextObject = currObject;
        nextShadingType = currShadingType;
        nextShader = currShader;
        nextRenderMode = currRenderMode;

        ////////// input controls //////////
        lastTime = currentTime;
        currentTime = (float) glfwGetTime();
        deltaTime = currentTime - lastTime;

        // close the window
        if (Input::IsKeyDown(GLFW_KEY_ESCAPE))
        {
            exit(0);
        }

        // Move forward
        if (Input::IsKeyDown(GLFW_KEY_W))
        {
            camera.MoveCamera(camera.GetCameraFront(), deltaTime * 5.0f);
        }
        // Move backward
        if (Input::IsKeyDown(GLFW_KEY_S))
        {
            camera.MoveCamera(-camera.GetCameraFront(), deltaTime * 5.0f);
        }
        // Strafe left
        if (Input::IsKeyDown(GLFW_KEY_A))
        {
            camera.MoveCamera(camera.GetCameraRight(), deltaTime * 5.0f);
        }
        // Strafe right
        if (Input::IsKeyDown(GLFW_KEY_D))
        {
            camera.MoveCamera(-camera.GetCameraRight(), deltaTime * 5.0f);
        }
        // fly up
        if (Input::IsKeyDown(GLFW_KEY_SPACE))
        {
            camera.MoveCamera(camera.GetCameraUp(), deltaTime * 5.0f);
        }
        // drop down
        if (Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
        {
            camera.MoveCamera(-camera.GetCameraUp(), deltaTime * 5.0f);
        }

        // move closer
        if (Input::IsKeyDown(GLFW_KEY_K))
        {
            float deltaDist = -deltaTime;
            camera.RotateCamera(0, 0, deltaDist);
        }
        // move further
        if (Input::IsKeyDown(GLFW_KEY_J))
        {
            float deltaDist = deltaTime;
            camera.RotateCamera(0, 0, deltaDist);
        }
        // rotate left
        if (Input::IsKeyDown(GLFW_KEY_LEFT))
        {
            float deltaYaw = deltaTime;
            camera.RotateCamera(0, deltaYaw, 0);
        }
        // rotate right
        if (Input::IsKeyDown(GLFW_KEY_RIGHT))
        {
            float deltaYaw = -deltaTime;
            camera.RotateCamera(0, deltaYaw, 0);
        }
        // rotate up
        if (Input::IsKeyDown(GLFW_KEY_UP))
        {
            float deltaPitch = deltaTime;
            camera.RotateCamera(deltaPitch, 0, 0);
        }
        // rotate down
        if (Input::IsKeyDown(GLFW_KEY_DOWN))
        {
            float deltaPitch = -deltaTime;
            camera.RotateCamera(deltaPitch, 0, 0);
        }
        
        // mouse movement
        glfwGetCursorPos(windowID, &currXpos, &currYpos);
        deltaX = (currXpos - lastXpos) / screenWidth;  // it is bounded by -1 and 1
        deltaY = (currYpos - lastYpos) / screenHeight; // it is bounded by -1 and 1
        lastXpos = currXpos;
        lastYpos = currYpos;

        // rotate model according to mouse movement
        if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_1) && !ImGui::GetIO().WantCaptureMouse)
        {
            rotationAngle = (float)deltaX * sens;
            rotationAngle > 360.0f ? rotationAngle -= 360.0f : NULL;
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // adjust FOV using vertical scroll
        camera.m_FOV -= Input::GetScrollY() * 2.0f;
        camera.m_FOV < 20.0f ? camera.m_FOV = 20.0f : NULL;
        camera.m_FOV > 110.0f ? camera.m_FOV = 110.0f : NULL;
        projMatrix = glm::perspective(glm::radians(camera.m_FOV), aspectRatio, 0.1f, 1000.0f);
        Input::ResetScroll();

        ////////// clearing per frame //////////
        glClearColor(0.80f, 0.90f, 0.96f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ////////// UI controls //////////
        // imgui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Display controls");
        if (ImGui::CollapsingHeader("Objection selection"))
        {
            ImGui::Indent();
            if (ImGui::CollapsingHeader("Geometric objects"))
            {
                ImGui::Indent();

                ImGui::RadioButton("Crumbled", &nextObject, CRUMPLED);
                ImGui::RadioButton("Cube", &nextObject, CUBE);
                ImGui::RadioButton("Cube2", &nextObject, CUBE2);  // temp
                ImGui::RadioButton("Double torus", &nextObject, DOUBLETORUS);
                ImGui::RadioButton("Fandisk", &nextObject, FANDISK);
                ImGui::RadioButton("Icosahedron", &nextObject, ICOSA);
                ImGui::RadioButton("Octahedron", &nextObject, OCTA);
                ImGui::RadioButton("Oloid", &nextObject, OLOID);
                ImGui::RadioButton("Sphere", &nextObject, SPHERE);
                ImGui::RadioButton("Star", &nextObject, STAR);
                ImGui::RadioButton("T-Shape", &nextObject, T);
                ImGui::RadioButton("T-Shape2", &nextObject, T2);  // temp
                ImGui::RadioButton("Torus", &nextObject, TORUS);
                ImGui::RadioButton("Tubes", &nextObject, TUBES);
                ImGui::RadioButton("Tubes2", &nextObject, TUBES2);  // temp

                ImGui::Unindent();
            }
            if (ImGui::CollapsingHeader("Model objects"))
            {
                ImGui::Indent();

                ImGui::RadioButton("Ankylosaurus", &nextObject, ANKYLOSAURUS);
                ImGui::RadioButton("Armadillo", &nextObject, ARMADILLO);
                ImGui::RadioButton("Bob", &nextObject, BOB);
                ImGui::RadioButton("Bob2", &nextObject, BOB2);  // temp
                ImGui::RadioButton("Bunny", &nextObject, BUNNY);
                ImGui::RadioButton("Cow", &nextObject, COW);
                ImGui::RadioButton("Cow2", &nextObject, COW2);  // temp
                ImGui::RadioButton("Cow head", &nextObject, COWHEAD);
                ImGui::RadioButton("Face", &nextObject, FACE);
                ImGui::RadioButton("Gargoyle", &nextObject, GARGOYLE);
                ImGui::RadioButton("Kitten", &nextObject, KITTEN);
                ImGui::RadioButton("Shuttle", &nextObject, SHUTTLE);
                ImGui::RadioButton("Suzanne", &nextObject, SUZANNE);
                ImGui::RadioButton("Teapot", &nextObject, TEAPOT);
                ImGui::RadioButton("Teddy", &nextObject, TEDDY);

                ImGui::Unindent();
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Modify Model"))
        {
            ImGui::Indent();
            if (ImGui::Button("Original"))
            {
                obj = objects.findObj(currObject); // search for the object requested
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Beehive Surface (tri)"))
            {
                Surface BH(obj);
                obj = BH.Beehive(3); 
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Beehive Surface (quad)"))
            {
                Surface BH(obj);
                obj = BH.Beehive(4);
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("SnowFlake Surface (tri)"))
            {
                Surface SF(obj);
                obj = SF.Snowflake(3);
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("SnowFlake Surface (quad)"))
            {
                Surface SF(obj);
                obj = SF.Snowflake(4);
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Catmull Clark Subdivision Surface (tri)"))
            {
                Surface CC(obj);
                obj = CC.CatmullClark(3);
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Catmull Clark Subdivision Surface (quad)"))
            {
                Surface CC(obj);
                obj = CC.CatmullClark(4);
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Doo Sabin Subdivision Surface"))
            {
                Surface DS(obj);
                obj = DS.DooSabin();
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Loop Subdivision Surface (tri)"))
            {
                Surface Lo(obj);
                obj = Lo.Loop3();
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            if (ImGui::Button("Loop Subdivision Surface (quad)"))
            {
                Surface Lo(obj);
                obj = Lo.Loop4();
                mesh.Rebuild(obj); // rebuild mesh based on object info
                numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

                objectVA.Bind();
                objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
                objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            }
            //if (ImGui::Button("Garland Heckbert Simplication Surface"))
            //{
            //    Surface GH(obj);
            //    obj = GH.QEM();
            //    mesh.Rebuild(obj); // rebuild mesh based on object info
            //    numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

            //    objectVA.Bind();
            //    objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            //    objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
            //}
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Shading type"))
        {
            ImGui::Indent();

            ImGui::RadioButton("Flat shading", &nextShadingType, FLAT);
            ImGui::RadioButton("Mixed shading", &nextShadingType, MIXED);
            ImGui::RadioButton("Smooth shading", &nextShadingType, SMOOTH);

            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Shader selection"))
        {
            ImGui::Indent();

            ImGui::RadioButton("Normal shader", &nextShader, NORMAL);
            ImGui::RadioButton("Gourand shader", &nextShader, GOURAND);
            ImGui::RadioButton("Phong shader", &nextShader, PHONG);
            ImGui::RadioButton("Blinn-Phong shader", &nextShader, BLINNPHONG);
            ImGui::RadioButton("Gooch shader", &nextShader, GOOCH);
            ImGui::RadioButton("Cel shader", &nextShader, CEL);
        
            if (nextShader == GOURAND || nextShader == PHONG || nextShader == BLINNPHONG || nextShader == GOOCH || nextShader == CEL)
            {
                ImGui::Indent();
                if (ImGui::CollapsingHeader("Material controls"))
                {
                    ImGui::Indent();

                    ImGui::ColorEdit3("Ambient color", meshMat.m_Ambient);
                    ImGui::ColorEdit3("Diffuse color", meshMat.m_Diffuse);
                    ImGui::ColorEdit3("Specular color", meshMat.m_Specular);
                    ImGui::SliderFloat("Shine constant", &meshMat.m_Shine, 10, 100);

                    ImGui::Unindent();
                }

                if (ImGui::CollapsingHeader("Lighting controls"))
                {
                    ImGui::Indent();

                    for (int l = 0; l < 3; l++)
                    {
                        ImGui::PushID(l);
                        ImGui::Text("Light #%d", l + 1);
                        ImGui::Checkbox("Toggle light", &light.m_LightsToggled[l]);
                        if (light.m_LightsToggled[l])
                        {
                            ImGui::SliderFloat3("position", &light.m_Pos[3 * l], -10, 10);
                            ImGui::ColorEdit3("color", &light.m_Col[3 * l]);

                            ImGui::Spacing();
                        }
                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                }

                if (nextShader == GOOCH)
                {
                    if (ImGui::CollapsingHeader("Gooch controls"))
                    {
                        ImGui::Indent();

                        ImGui::ColorEdit3("Warm color", gooch_warm);
                        ImGui::ColorEdit3("Cool color", gooch_cool);
                        ImGui::SliderFloat("Alpha", &gooch_alpha, 0, 1);
                        ImGui::SliderFloat("Beta", &gooch_beta, 0, 1);

                        ImGui::Spacing();

                        ImGui::Unindent();
                    }
                }

                ImGui::Unindent();
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Render mode"))
        {
            ImGui::Indent();
            
            ImGui::RadioButton("Polygon", &nextRenderMode, POLYGON);
            ImGui::RadioButton("Wireframe", &nextRenderMode, WIREFRAME);
            ImGui::RadioButton("Point cloud", &nextRenderMode, POINTCLOUD);
                
            ImGui::Unindent();
        }
        
        if (ImGui::Button("Reset Camera"))
        {
            camera.ResetView();
        }
        ImGui::End();

        ImGui::Begin("Utilities");
        if (ImGui::CollapsingHeader("Statistics"))
        {
            ImGui::Indent();

            ImGui::Checkbox("Framerate tracker", &framerate);
            if (framerate)
            {
                ImGui::Text("Application average %.1f FPS: ", ImGui::GetIO().Framerate);
            }
            ImGui::Checkbox("Number of Polygons", &triangles);
            if (triangles)
            {
                std::stringstream ss;
                ss << numFaces;
                std::string sstr = "Number of polygons: " + ss.str();
                ImGui::Text(sstr.c_str());

                for (const std::pair<int, int> numPolygon : mesh.m_Object.m_NumPolygons)
                {
                    std::stringstream sizeStringStream;
                    sizeStringStream << numPolygon.first;
                    std::stringstream numStringStream;
                    numStringStream << numPolygon.second;
                    std::string str = "Number of " + sizeStringStream.str() + "-gons: " + numStringStream.str();
                    ImGui::Text(str.c_str());
                }
            }

            ImGui::Unindent();
        }

        if (ImGui::Button("Screenshot"))
        {
            struct tm newtime;
            time_t now = time(0);
            localtime_s(&newtime, &now);

            // print various components of tm structure.
            std::string year = std::to_string(1900 + newtime.tm_year);
            std::string month = std::to_string(1 + newtime.tm_mon);
            std::string day = std::to_string(newtime.tm_mday);
            std::string hour = std::to_string(newtime.tm_hour);
            std::string min = std::to_string(newtime.tm_min);
            std::string sec = std::to_string(newtime.tm_sec);

            std::string path = "gallery/Screenshot_" + year + "-" + month + "-" + day + "_" + hour + min + sec + ".png";
            saveImage(path.c_str(), windowID);
        }

        ImGui::End();

        ImGui::EndFrame();
        ImGui::Render();

        ////////// change shader //////////
        if (nextShader != currShader)
        {
            currShader = nextShader;

            if (currShader == PHONG)
                shader = phongShader;
            else if (currShader == BLINNPHONG)
                shader = blinnPhongShader;
            else if (currShader == GOURAND)
                shader = gourandShader;
            else if (currShader == NORMAL)
                shader = normalShader;
            else if (currShader == GOOCH)
                shader = goochShader;
            else if (currShader == CEL)
                shader = celShader;

            shader.Bind();
        }

        ////////// upload uniforms //////////
        shader.SetUniformMat4f("u_Model", modelMatrix);
        shader.SetUniformMat4f("u_View", camera.GetViewMatrix());
        shader.SetUniformMat4f("u_Projection", projMatrix);
        if (currShader == GOURAND || currShader == PHONG || currShader == BLINNPHONG || currShader == GOOCH || currShader == CEL)
        {
            shader.SetUniform3fv("light_pos", 3, light.m_Pos);
            shader.SetUniform3fv("light_col", 3, light.m_Col); 
            ////////// cast bool to int /////////
            for (int i = 0; i < 3; i++)
            {
                toggled[i] = light.m_LightsToggled[i];
            }
            shader.SetUniform1iv("light_toggled", 3, toggled); // upload the lights toggle option

            shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
            shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
            shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
            shader.SetUniform1f("shine", meshMat.m_Shine);

            if (currShader == GOOCH)
            {
                shader.SetUniform3fv("warm", 1, gooch_warm);
                shader.SetUniform3fv("cool", 1, gooch_cool);
                shader.SetUniform1f("alpha", gooch_alpha);
                shader.SetUniform1f("beta", gooch_beta);
            }
        }

        ////////// regenerate object //////////
        if (nextShadingType != currShadingType)
        {
            currShadingType = nextShadingType;

            mesh.Rebuild(currShadingType); // rebuild mesh based on shading type

            objectVA.Bind();
            objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
        }

        ////////// regenerate object //////////
        if (nextObject != currObject)
        {
            currObject = nextObject;

            obj = objects.findObj(currObject); // search for the object requested
            
            // TODO: make display object?
            mesh.Rebuild(obj); // rebuild mesh based on object info
            numFaces = static_cast<int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

            objectVA.Bind();
            objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
        }

        ////////// change render mode //////////
        if (nextRenderMode != currRenderMode)
        {
            currRenderMode = nextRenderMode;

            if (currRenderMode == POLYGON)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            else if (currRenderMode == WIREFRAME)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else if (currRenderMode == POINTCLOUD)
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        }

        ////////// Render object here //////////
        glDrawElements(GL_TRIANGLES, objectIB.GetCount(), GL_UNSIGNED_INT, 0);

        ////////// Render Imgui here //////////
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(windowID);

        /* Poll for and process events */
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    delete[] toggled;

    window.~Window();
    return 0;
}