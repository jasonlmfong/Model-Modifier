#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_glfw.h"
#include "external/imgui/imgui_impl_opengl3.h"

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

int main()
{
    unsigned int screenWidth = 1440;
    unsigned int screenHeight = 810;
    float aspectRatio = (float)screenWidth / screenHeight;
    Window window(screenWidth, screenHeight, "Model Modifier", NULL);

    // build mesh from obj file
    Mesh mesh("res/objects/bunny.obj");
    Material meshMat;

    VertexBufferLayout layout;
    layout.Push<float>(3); // 3d coordinates
    layout.Push<float>(3); // normals

    // build openGL objects using mesh
    VertexArray objectVA;
    VertexBuffer objectVB(mesh.m_Vertices, 2 * 3 * mesh.m_VertexPos.size() * sizeof(float), DRAW_MODE::STATIC);
    // bind vertex buffer to vertex array
    objectVA.AddBuffer(objectVB, layout);
    IndexBuffer objectIB(mesh.m_Indices, 3 * mesh.m_Elements.size(), DRAW_MODE::STATIC);

    // shaders
    std::string vertexFilepath = "res/shaders/phong.vert";
    std::string fragmentFilepath = "res/shaders/phong.frag";

    Shader shader(vertexFilepath, fragmentFilepath);

    // camera setup
    float FOV = 30.0f;
    double yaw = -90.0;
    double pitch = -30.0;
    glm::vec3 cameraPosition = { 0.0f, 0.65f, 1.0f };
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
    shader.SetUniform3fv("light_pos", 3, light.m_Pos);
    shader.SetUniform3fv("light_col", 3, light.m_Col);
    shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
    shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
    shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
    shader.SetUniform1f("shine", meshMat.m_Shine);

    // openGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        ////////// input controls //////////

        lastTime = currentTime;
        currentTime = glfwGetTime();
        deltaTime = float(currentTime - lastTime);

        // close the window
        if (Input::IsKeyDown(GLFW_KEY_Q))
        {
            exit(0);
        }
        // rotate left
        if (Input::IsKeyDown(GLFW_KEY_LEFT))
        {
            rotationAngle = (float)-0.25f;
            rotationAngle > 360.0f ? rotationAngle -= 360.0f : NULL;
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.SetUniformMat4f("u_Model", modelMatrix);
        }
        // rotate right
        if (Input::IsKeyDown(GLFW_KEY_RIGHT))
        {
            rotationAngle = (float)0.25f;
            rotationAngle > 360.0f ? rotationAngle -= 360.0f : NULL;
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.SetUniformMat4f("u_Model", modelMatrix);
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
            shader.SetUniformMat4f("u_Model", modelMatrix);
        }

        // adjust FOV using vertical scroll
        FOV -= Input::GetScrollY() * 2.0f;
        FOV < 10.0f ? FOV = 10.0f : NULL;
        FOV > 65.0f ? FOV = 65.0f : NULL;
        projMatrix = glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 1000.0f);
        shader.SetUniformMat4f("u_Projection", projMatrix);
        Input::ResetScroll();

        ////////// clearing per frame //////////
        glClearColor(0.80f, 0.90f, 0.96f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ////////// Render here //////////
        objectVA.Bind();
        shader.Bind();
        glDrawElements(GL_TRIANGLES, objectIB.GetCount(), GL_UNSIGNED_INT, 0);

        ////////// UI controls //////////
        // imgui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Display parameters");
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
        ImGui::Checkbox("Material controls", &mat);
        if (mat)
        {
            ImGui::ColorEdit3("ambient", meshMat.m_Ambient);
            ImGui::ColorEdit3("diffuse", meshMat.m_Diffuse);
            ImGui::ColorEdit3("specular", meshMat.m_Specular);
            ImGui::SliderFloat("shine", &meshMat.m_Shine, 10, 100);
        }
        ImGui::Checkbox("Lighting controls", &lighting);
        if (lighting)
        {
            for (int l = 0; l < 3; l++)
            {
                ImGui::PushID(l);
                ImGui::Text("Light #%d", l + 1);
                ImGui::SliderFloat3("position", &light.m_Pos[3 * l], -10, 10);
                ImGui::ColorEdit3("color", &light.m_Col[3 * l]);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::End();

        ////////// upload uniforms //////////
        shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient);
        shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse);
        shader.SetUniform3fv("specular", 1, meshMat.m_Specular);
        shader.SetUniform1f("shine", meshMat.m_Shine);
        
        shader.SetUniform3fv("light_pos", 3, light.m_Pos);
        shader.SetUniform3fv("light_col", 3, light.m_Col);

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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