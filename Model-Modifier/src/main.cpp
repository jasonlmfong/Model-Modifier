#include <iostream>
#include <ctime>

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
#include "scene/Mesh.h"
#include "scene/Material.h"
#include "scene/Light.h"

enum object
{
    BUNNY,
    COWHEAD,
    DOUBLETORUS,
    FACE,
    GARGOYLE,
    ICOSA,
    KITTEN,
    SHUTTLE,
    SPHERE,
    SUZANNE,
    TEAPOT,
    TEDDY,
    TORUS
};

enum shader
{
    PHONG,
    NORMAL
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

    // build mesh from obj file
    int currObject = BUNNY;
    int nextObject;
    Mesh mesh("res/objects/bunny.obj");
    Material meshMat;

    VertexBufferLayout layout;
    layout.Push<float>(3); // 3d coordinates
    layout.Push<float>(3); // normals

    // shading type
    int currShadingType = FLAT;
    int nextShadingType;
    mesh.preRender(currObject);

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

    std::string normalVertexPath = "res/shaders/normal.vert";
    std::string normalFragmentPath = "res/shaders/normal.frag";
    Shader normalShader(normalVertexPath, normalFragmentPath);

    int currShader = NORMAL;
    int nextShader;
    Shader shader = normalShader;

    // camera setup
    float FOV = 65.0f;
    double yaw = -90.0;
    double pitch = -30.0;
    glm::vec3 cameraPosition = { 0.0f, 1.75f, 2.0f };
    Camera camera(cameraPosition, yaw, pitch);

    float rotationAngle = 0.0f; // rotation angle of the object mesh
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projMatrix = glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 1000.0f);

    // lighting
    Light light = Light(glm::vec3(0, 10, 0), glm::vec3(1.0f, 1.0f, 1.0f));

    // upload uniforms
    shader.Bind();
    shader.SetUniformMat4f("u_Model", modelMatrix);
    shader.SetUniformMat4f("u_View", camera.GetViewMatrix());
    shader.SetUniformMat4f("u_Projection", projMatrix);
    if (currShader == PHONG)
    {
        shader.SetUniform3fv("light_pos", 3, light.m_Pos);
        shader.SetUniform3fv("light_col", 3, light.m_Col);

        shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
        shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
        shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
        shader.SetUniform1f("shine", meshMat.m_Shine);
    }

    // openGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // object selection
    bool objSelect = false;
    // wireframe mode
    bool wireframe = false;
    // framerate mode
    bool framerate = false;
    // lighting controls
    bool lighting = false;
    // material controls
    bool mat = false;

    GLFWwindow* windowID = window.GetID();
    // input initialization & input callbacks
    Input::Init(windowID);

    // keyboard movement variables
    double currentTime = 0.0;
    double lastTime = 0.0;
    float deltaTime = 0.0f;
    // mouse movement variables
    double currXpos, currYpos, deltaX, deltaY;
    double lastXpos = 0.0;
    double lastYpos = 0.0;
    double sens = 200.0;

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

        ////////// input controls //////////
        lastTime = currentTime;
        currentTime = glfwGetTime();
        deltaTime = float(currentTime - lastTime);

        // close the window
        if (Input::IsKeyDown(GLFW_KEY_ESCAPE))
        {
            exit(0);
        }
        // rotate left
        if (Input::IsKeyDown(GLFW_KEY_LEFT))
        {
            rotationAngle = (float)-0.25f;
            rotationAngle > 360.0f ? rotationAngle -= 360.0f : NULL;
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        // rotate right
        if (Input::IsKeyDown(GLFW_KEY_RIGHT))
        {
            rotationAngle = (float)0.25f;
            rotationAngle > 360.0f ? rotationAngle -= 360.0f : NULL;
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        // Move forward
        if (Input::IsKeyDown(GLFW_KEY_W))
        {
            camera.MoveCamera(camera.GetCameraFront(), deltaTime * 5.0f);
            camera.SetViewMatrix();
        }
        // Move backward
        if (Input::IsKeyDown(GLFW_KEY_S))
        {
            camera.MoveCamera(-camera.GetCameraFront(), deltaTime * 5.0f);
            camera.SetViewMatrix();
        }
        // Strafe left
        if (Input::IsKeyDown(GLFW_KEY_A))
        {
            camera.MoveCamera(camera.GetCameraRight(), deltaTime * 5.0f);
            camera.SetViewMatrix();
        }
        // Strafe right
        if (Input::IsKeyDown(GLFW_KEY_D))
        {
            camera.MoveCamera(-camera.GetCameraRight(), deltaTime * 5.0f);
            camera.SetViewMatrix();
        }
        // fly up
        if (Input::IsKeyDown(GLFW_KEY_SPACE))
        {
            camera.MoveCamera(camera.GetCameraUp(), deltaTime * 5.0f);
            camera.SetViewMatrix();
        }
        // drop down
        if (Input::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            camera.MoveCamera(-camera.GetCameraUp(), deltaTime * 5.0f);
            camera.SetViewMatrix();
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
        FOV -= Input::GetScrollY() * 2.0f;
        FOV < 20.0f ? FOV = 20.0f : NULL;
        FOV > 110.0f ? FOV = 110.0f : NULL;
        projMatrix = glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 1000.0f);
        Input::ResetScroll();

        ////////// clearing per frame //////////
        glClearColor(0.80f, 0.90f, 0.96f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ////////// UI controls //////////
        // imgui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Display parameters");
        ImGui::Checkbox("Objection Selection", &objSelect);
        if (objSelect)
        {
            ImGui::RadioButton("Bunny", &nextObject, BUNNY);
            ImGui::RadioButton("Cow head", &nextObject, COWHEAD);
            ImGui::RadioButton("Double torus", &nextObject, DOUBLETORUS);
            ImGui::RadioButton("Face", &nextObject, FACE);
            ImGui::RadioButton("Gargoyle", &nextObject, GARGOYLE);
            ImGui::RadioButton("Icosahedron", &nextObject, ICOSA);
            ImGui::RadioButton("Kitten", &nextObject, KITTEN);
            ImGui::RadioButton("Shuttle", &nextObject, SHUTTLE);
            ImGui::RadioButton("Sphere", &nextObject, SPHERE);
            ImGui::RadioButton("Suzanne", &nextObject, SUZANNE);
            ImGui::RadioButton("Teapot", &nextObject, TEAPOT);
            ImGui::RadioButton("Teddy", &nextObject, TEDDY);
            ImGui::RadioButton("Torus", &nextObject, TORUS);
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Shading Type");
        ImGui::RadioButton("Flat Shading", &nextShadingType, FLAT);
        ImGui::RadioButton("Smooth Shading", &nextShadingType, SMOOTH);

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Shader Selection");
        ImGui::RadioButton("Normal Shader", &nextShader, NORMAL);
        ImGui::RadioButton("Phong Shader", &nextShader, PHONG);

        if (nextShader == PHONG)
        {
            ImGui::Checkbox("Material Controls", &mat);
            if (mat)
            {
                ImGui::ColorEdit3("ambient", meshMat.m_Ambient);
                ImGui::ColorEdit3("diffuse", meshMat.m_Diffuse);
                ImGui::ColorEdit3("specular", meshMat.m_Specular);
                ImGui::SliderFloat("shine", &meshMat.m_Shine, 10, 100);
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Checkbox("Lighting Controls", &lighting);
            if (lighting)
            {
                for (int l = 0; l < 3; l++)
                {
                    ImGui::PushID(l);
                    ImGui::Text("Light #%d", l + 1);
                    ImGui::SliderFloat3("position", &light.m_Pos[3 * l], -10, 10);
                    ImGui::ColorEdit3("color", &light.m_Col[3 * l]);

                    ImGui::Spacing();
                    ImGui::PopID();
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Options:");
        if (ImGui::Checkbox("Wireframe Mode", &wireframe))
        {
            if (wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // surface mode
        }
        ImGui::Checkbox("Framerate Tracker", &framerate);
        if (framerate)
        {
            ImGui::Text("Application average %.1f FPS", ImGui::GetIO().Framerate);
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

            std::string path = "gallery/Screenshot " + year + "-" + month + "-" + day + " " + hour + min + sec + ".png";
            saveImage(path.c_str(), windowID);
        }

        ImGui::End();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ////////// change shader //////////
        if (nextShader != currShader)
        {
            currShader = nextShader;

            if (currShader == PHONG)
                shader = phongShader;
            else if (currShader == NORMAL)
                shader = normalShader;

            shader.Bind();
        }

        ////////// upload uniforms //////////
        shader.SetUniformMat4f("u_Model", modelMatrix);
        shader.SetUniformMat4f("u_View", camera.GetViewMatrix());
        shader.SetUniformMat4f("u_Projection", projMatrix);
        if (currShader == PHONG)
        {
            shader.SetUniform3fv("light_pos", 3, light.m_Pos);
            shader.SetUniform3fv("light_col", 3, light.m_Col);

            shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
            shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
            shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
            shader.SetUniform1f("shine", meshMat.m_Shine);
        }

        ////////// regenerate object //////////
        if (nextShadingType != currShadingType)
        {
            currShadingType = nextShadingType;

            mesh.preRender(currShadingType);

            objectVA.Bind();
            objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
        }

        ////////// regenerate object //////////
        if (nextObject != currObject)
        {
            currObject = nextObject;

            switch (currObject)
            {
                case BUNNY:
                {
                    mesh.Reload("res/objects/bunny.obj");
                    break;
                }
                case COWHEAD:
                {
                    mesh.Reload("res/objects/cowhead.obj");
                    break;
                }
                case DOUBLETORUS:
                {
                    mesh.Reload("res/objects/double-torus.obj");
                    break;
                }
                case FACE:
                {
                    mesh.Reload("res/objects/face.obj");
                    break;
                }
                case GARGOYLE:
                {
                    mesh.Reload("res/objects/gargoyle.obj");
                    break;
                }
                case ICOSA:
                {
                    mesh.Reload("res/objects/ico.obj");
                    break;
                }
                case KITTEN:
                {
                    mesh.Reload("res/objects/kitten.obj");
                    break;
                }
                case SHUTTLE:
                {
                    mesh.Reload("res/objects/shuttle.obj");
                    break;
                }
                case SPHERE:
                {
                    mesh.Reload("res/objects/sphere.obj");
                    break;
                }
                case SUZANNE:
                {
                    mesh.Reload("res/objects/suzanne.obj");
                    break;
                }
                case TEAPOT:
                {
                    mesh.Reload("res/objects/teapot.obj");
                    break;
                }
                case TEDDY:
                {
                    mesh.Reload("res/objects/teddy.obj");
                    break;
                }
                case TORUS:
                {
                    mesh.Reload("res/objects/torus.obj");
                    break;
                }
            }

            mesh.preRender(currShadingType);
            objectVA.Bind();
            objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);
        }

        ////////// Render here //////////
        glDrawElements(GL_TRIANGLES, objectIB.GetCount(), GL_UNSIGNED_INT, 0);

        /* Swap front and back buffers */
        glfwSwapBuffers(windowID);

        /* Poll for and process events */
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    window.~Window();
    return 0;
}