#include <iostream>
#include <ctime>
#include <string>
#include <vector>
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
#include "renderer/SaveImage.h"
#include "scene/object/Object.h"
#include "scene/object/ObjectSelect.h"
#include "scene/Mesh.h"
#include "scene/Material.h"
#include "scene/Light.h"
#include "scene/surface/Surface.h"

#include "ImguiSections.h"

enum shaderEnum
{
    GOURAND,
    NORMAL,
    PHONG,
    BLINNPHONG,
    GOOCH,
    CEL,
    COOKTORRANCE,
};

enum renderMode
{
    POLYGON,
    WIREFRAME,
    POINTCLOUD
};

static void processKeyboardInput(GLFWwindow* window, Camera& camera, float deltaTime)
{
    // close the window
    if (Input::IsKeyDown(GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(window, true);
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
}

int main()
{
    unsigned int screenWidth = 1440;
    unsigned int screenHeight = 810;
    float aspectRatio = (float)screenWidth / screenHeight;
    Window window(screenWidth, screenHeight, "Model Modifier", NULL);

    // build object from obj file
    ObjectSelect objects;
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
    unsigned int numFaces = static_cast<unsigned int>(mesh.m_Object.m_FaceIndices.size());

    // build openGL objects using mesh
    VertexArray objectVA;
    VertexBuffer objectVB(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
    // bind vertex buffer to vertex array
    objectVA.AddBuffer(objectVB, layout);
    IndexBuffer objectIB(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);

    // shaders
    std::string phongVertexPath = "res/shaders/phong.vert";
    std::string phongFragmentPath = "res/shaders/phong.frag";
    ShaderProgram phongShader(phongVertexPath, phongFragmentPath);
    
    std::string blinnPhongFragmentPath = "res/shaders/blinnPhong.frag";
    ShaderProgram blinnPhongShader(phongVertexPath, blinnPhongFragmentPath);

    std::string normalVertexPath = "res/shaders/normal.vert";
    std::string normalFragmentPath = "res/shaders/normal.frag";
    ShaderProgram normalShader(normalVertexPath, normalFragmentPath);

    std::string gourandVertexPath = "res/shaders/gourand.vert";
    std::string gourandFragmentPath = "res/shaders/gourand.frag";
    ShaderProgram gourandShader(gourandVertexPath, gourandFragmentPath);

    std::string goochFragmentPath = "res/shaders/gooch.frag";
    ShaderProgram goochShader(phongVertexPath, goochFragmentPath);

    std::string celFragmentPath = "res/shaders/cel.frag";
    ShaderProgram celShader(phongVertexPath, celFragmentPath);

    std::string cookTorranceFragmentPath = "res/shaders/cookTorrance.frag";
    ShaderProgram cookTorranceShader(phongVertexPath, cookTorranceFragmentPath);

    int currShader = NORMAL;
    int nextShader;
    ShaderProgram shader = normalShader;
    shader.Bind();

    // camera setup
    float yaw = 1.5f; // radians
    float pitch = 0.3333f; // radians
    float radius = 3.0f;
    Camera camera(pitch, yaw, radius);

    float rotationAngle = 0.0f; // rotation angle of the object mesh
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 projMatrix = glm::perspective(glm::radians(camera.m_FOV), aspectRatio, 0.1f, 1000.0f);

    // lighting
    Light light = Light();
    std::vector<int> toggled; toggled.resize(3);

    // Gooch variables
    std::vector<float> gooch_warm = {1, 0.25, 0};
    std::vector<float> gooch_cool = {0, 0.75, 1};
    float gooch_alpha = 0.2f;
    float gooch_beta = 0.2f;

    // Cook-Torrance variables
    float metallic = 0.2f;
    float roughness = 0.3f;

    // Apply modification algorithm
    bool ModifyModel = false;

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

        processKeyboardInput(windowID, camera, deltaTime);
        
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
        if (Input::GetScrollY() != 0)
        {
            camera.changeFOV(Input::GetScrollY());
            projMatrix = glm::perspective(glm::radians(camera.m_FOV), aspectRatio, 0.1f, 1000.0f);
            Input::ResetScroll();
        }

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
                for (std::pair<unsigned int, const char *> displayName : geometricObjectNames)
                {
                    ImGui::RadioButton(displayName.second, &nextObject, displayName.first);
                }
                ImGui::Unindent();
            }
            if (ImGui::CollapsingHeader("Model objects"))
            {
                ImGui::Indent();
                for (std::pair<unsigned int, const char*> displayName : modelObjectNames)
                {
                    ImGui::RadioButton(displayName.second, &nextObject, displayName.first);
                }
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
                ModifyModel = true;
            }
            if (ImGui::Button("Triangulate Surface"))
            {
                obj.MakeTriangleMesh();
                ModifyModel = true;
            }
            if (ImGui::Button("Beehive Surface"))
            {
                Surface BH(obj);
                obj = BH.Beehive();
                ModifyModel = true;
            }
            if (ImGui::Button("SnowFlake Surface"))
            {
                Surface SF(obj);
                obj = SF.Snowflake();
                ModifyModel = true;
            }
            if (ImGui::Button("Catmull Clark Subdivision Surface"))
            {
                Surface CC(obj);
                obj = CC.CatmullClark();
                ModifyModel = true;
            }
            if (ImGui::Button("Doo Sabin Subdivision Surface"))
            {
                Surface DS(obj);
                obj = DS.DooSabin();
                ModifyModel = true;
            }
            if (ImGui::Button("Loop Subdivision Surface"))
            {
                Surface Lo(obj);
                obj = Lo.Loop();
                ModifyModel = true;
            }
            //if (ImGui::Button("Garland Heckbert Simplication Surface"))
            //{
            //    Surface GH(obj);
            //    obj = GH.QEM();
            //    mesh.Rebuild(obj); // rebuild mesh based on object info
            //    numFaces = static_cast<unsigned int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

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
            ImGui::RadioButton("Cook-Torrance shader", &nextShader, COOKTORRANCE);
            ImGui::RadioButton("Cel shader", &nextShader, CEL);
            ImGui::RadioButton("Gooch shader", &nextShader, GOOCH);
        
            if (nextShader == GOURAND || nextShader == PHONG || nextShader == BLINNPHONG || nextShader == GOOCH || nextShader == CEL || nextShader == COOKTORRANCE)
            {
                ImGui::Indent();
                if (ImGui::CollapsingHeader("Material controls"))
                {
                    ImGui::Indent();

                    ImGui::ColorEdit3("Ambient color", meshMat.m_Ambient.data());
                    ImGui::ColorEdit3("Diffuse color", meshMat.m_Diffuse.data());
                    ImGui::ColorEdit3("Specular color", meshMat.m_Specular.data());
                    if (nextShader != COOKTORRANCE)
                        ImGui::SliderFloat("Shine constant", &meshMat.m_Shine, 10, 100);

                    ImGui::Unindent();
                }

                if (ImGui::CollapsingHeader("Lighting controls"))
                {
                    ImGui::Indent();

                    for (unsigned int l = 0; l < 3; l++)
                    {
                        ImGui::PushID(l);
                        ImGui::Text("Light #%d", l + 1);
                        ImGui::Checkbox("Toggle light", &light.m_LightsToggled[l]);
                        if (light.m_LightsToggled[l])
                        {
                            ImGui::SliderFloat3("position", &light.m_Pos.data()[3 * l], -10, 10);
                            ImGui::ColorEdit3("color", &light.m_Col.data()[3 * l]);
                            ImGui::SliderFloat("brightness", &light.m_Brightness[l], 0, 2);

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

                        ImGui::ColorEdit3("Warm color", gooch_warm.data());
                        ImGui::ColorEdit3("Cool color", gooch_cool.data());
                        ImGui::SliderFloat("Alpha", &gooch_alpha, 0, 1);
                        ImGui::SliderFloat("Beta", &gooch_beta, 0, 1);

                        ImGui::Spacing();

                        ImGui::Unindent();
                    }
                }

                if (nextShader == COOKTORRANCE)
                {
                    if (ImGui::CollapsingHeader("Cook-Torrance controls"))
                    {
                        ImGui::Indent();

                        ImGui::SliderFloat("Metallic", &metallic, 0, 1);
                        ImGui::SliderFloat("Roughness", &roughness, 0.05f, 1);

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

                for (const std::pair<unsigned int, unsigned int> numPolygon : mesh.m_Object.m_NumPolygons)
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
            else if (currShader == COOKTORRANCE)
                shader = cookTorranceShader;

            shader.Bind();
        }

        ////////// upload uniforms //////////
        shader.SetUniformMat4f("u_Model", modelMatrix);
        shader.SetUniformMat4f("u_View", camera.GetViewMatrix());
        shader.SetUniformMat4f("u_Projection", projMatrix);
        if (currShader == GOURAND || currShader == PHONG || currShader == BLINNPHONG || currShader == GOOCH || currShader == CEL || currShader == COOKTORRANCE)
        {
            shader.SetUniform3fv("light_pos", 3, light.m_Pos.data());
            shader.SetUniform3fv("light_col", 3, light.m_Col.data());
            shader.SetUniform3f("light_brightness", light.m_Brightness[0], light.m_Brightness[1], light.m_Brightness[2]);
            ////////// cast bool to int /////////
            for (unsigned int i = 0; i < 3; i++)
            {
                toggled[i] = static_cast<int>(light.m_LightsToggled[i]);
            }
            shader.SetUniform1iv("light_toggled", 3, toggled.data()); // upload the lights toggle option

            shader.SetUniform3fv("ambient", 1, meshMat.m_Ambient.data());
            shader.SetUniform3fv("diffuse", 1, meshMat.m_Diffuse.data());
            shader.SetUniform3fv("specular", 1, meshMat.m_Specular.data());
            shader.SetUniform1f("shine", meshMat.m_Shine);

            if (currShader == GOOCH)
            {
                shader.SetUniform3fv("warm", 1, gooch_warm.data());
                shader.SetUniform3fv("cool", 1, gooch_cool.data());
                shader.SetUniform1f("alpha", gooch_alpha);
                shader.SetUniform1f("beta", gooch_beta);
            }
            if (currShader == COOKTORRANCE)
            {
                shader.SetUniform1f("metallic", metallic);
                shader.SetUniform1f("roughness", roughness);
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
            
            ModifyModel = true;
        }

        ////////// apply modification //////////
        if (ModifyModel)
        {
            mesh.Rebuild(obj); // rebuild mesh based on object info
            numFaces = static_cast<unsigned int>(mesh.m_Object.m_FaceIndices.size()); // update number of faces

            objectVA.Bind();
            objectVB.AssignData(mesh.m_OutVertices, mesh.m_OutNumVert * sizeof(float), DRAW_MODE::STATIC);
            objectIB.AssignData(mesh.m_OutIndices, mesh.m_OutNumIdx, DRAW_MODE::STATIC);

            ModifyModel = false;
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

    window.~Window();
    return 0;
}
