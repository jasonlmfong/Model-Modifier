#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stb_image_write.h>

#include "renderer/Window.h"
#include "renderer/Input.h"
#include "renderer/CameraController.h"
#include "renderer/SaveImage.h"
#include <OreRenderer/Mesh.h>
#include <OreRenderer/Renderer.h>
#include "scene/object/ObjectSelect.h"

#include "surface/Surface.h"

enum renderMode
{
    POLYGON,
    WIREFRAME,
    POINTCLOUD
};

int main()
{
    unsigned int screenWidth = 1440;
    unsigned int screenHeight = 810;
    float aspectRatio = (float)screenWidth / screenHeight;
    Window window(screenWidth, screenHeight, "Model Modifier", NULL);

    // build object from obj file
    ObjectSelect objects;
    int currObject = static_cast<int>(objects.findIndex("bunny"));
    int nextObject;
    Mesh mesh = objects.findMesh(currObject);


    // object triangle count (QEM)
    int triCount = static_cast<int>(mesh.m_TriFaceIndices.size());
    int desiredTriCount = triCount;


    // render mode
    int currRenderMode = POLYGON;    
    int nextRenderMode;
    glPointSize(2);

    // shading type
    int currShadingType = FLAT;
    int nextShadingType;

    mesh.m_ShadingType = currShadingType;
    // for rendering
    mesh.BuildFaceNormals();
    mesh.BuildVerticesIndices();

    Renderer renderer("res/shaders");
    renderer.SetMesh(mesh);
    renderer.SetProjectionMatrix(glm::perspective(glm::radians(renderer.GetCamera().m_FOV), aspectRatio, 0.1f, 1000.0f));

    Camera& camera = renderer.GetCamera();
    Light& light = renderer.GetLight();
    Material& material = renderer.GetMaterial();
    ShaderLibrary& shaders = renderer.GetShaderLibrary();
    GoochParams& gooch = renderer.GetGoochParams();
    PBRParams& pbr = renderer.GetPBRParams();

    // shader selection as int for imgui
    int currShader = static_cast<int>(ShaderType::Normal);
    int nextShader;
    shaders.SetActiveShader(ShaderType::Normal);

    // keep track of number of faces
    unsigned int numFaces = static_cast<unsigned int>(mesh.m_FaceIndices.size());

    glm::mat4 modelMatrix = glm::mat4(1.0f);

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
    Input::Init(windowID);
    CameraController cameraCtrl(camera, screenWidth, screenHeight);

    float currentTime = 0.0f;
    float lastTime = 0.0f;
    float deltaTime = 0.0f;

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
        currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastTime;

        cameraCtrl.Update(windowID, deltaTime);

        if (cameraCtrl.Rotated())
        {
            modelMatrix = cameraCtrl.ApplyModelRotation(modelMatrix);
        }

        if (cameraCtrl.ProjectionChanged())
        {
            renderer.SetProjectionMatrix(glm::perspective(glm::radians(camera.m_FOV), aspectRatio, 0.1f, 1000.0f));
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
            for (size_t i = 0; i < objects.count(); ++i)
            {
                ImGui::RadioButton(objects.name(i).c_str(), &nextObject, static_cast<int>(i));
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Modify Model"))
        {
            ImGui::Indent();

            if (ImGui::Button("Original"))
            {
                mesh = objects.findMesh(currObject); // search for the object requested
                ModifyModel = true;
            }
            if (ImGui::Button("Triangulate Surface"))
            {
                mesh.MakeTriangleMesh();
                ModifyModel = true;
            }
            if (ImGui::Button("Beehive Surface"))
            {
                SubdivideBeehive(mesh);
                ModifyModel = true;
            }
            if (ImGui::Button("Snowflake Surface"))
            {
                SubdivideSnowflake(mesh);
                ModifyModel = true;
            }
            if (ImGui::Button("Catmull Clark Subdivision"))
            {
                SubdivideCatmullClark(mesh);
                ModifyModel = true;
            }
            if (ImGui::Button("Doo Sabin Subdivision"))
            {
                SubdivideDooSabin(mesh);
                ModifyModel = true;
            }
            if (ImGui::Button("Loop Subdivision"))
            {
                SubdivideLoop(mesh);
                ModifyModel = true;
            }
            if (ImGui::Button("Garland Heckbert Simplification"))
            {
                mesh.MakeTriangleMesh(); // Triangulate first
                SimplifyGarlandHeckbert(mesh, desiredTriCount);
                ModifyModel = true;
            }
            ImGui::Indent();
            ImGui::SliderInt("Desired count##GH", &desiredTriCount, triCount / 5, triCount);
            ImGui::Unindent();

            static float lrzAlpha = 0.5f;
            if (ImGui::Button("Liu Rahimzadeh Zordan Simplification"))
            {
                mesh.MakeTriangleMesh(); // Triangulate first
                SimplifyLiuRahimzadehZordan(mesh, desiredTriCount, lrzAlpha);
                ModifyModel = true;
            }
            ImGui::Indent();
            ImGui::SliderInt("Desired count##LRZ", &desiredTriCount, triCount / 5, triCount);
            ImGui::SliderFloat("Alpha (edge preservation)", &lrzAlpha, 0.0f, 1.0f);
            ImGui::Text("0.0 = smooth, 0.5 = balanced, 1.0 = sharp edges");
            ImGui::Unindent();

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

            ImGui::RadioButton("Normal shader", &nextShader, static_cast<int>(ShaderType::Normal));
            ImGui::RadioButton("Gourand shader", &nextShader, static_cast<int>(ShaderType::Gourand));
            ImGui::RadioButton("Phong shader", &nextShader, static_cast<int>(ShaderType::Phong));
            ImGui::RadioButton("Blinn-Phong shader", &nextShader, static_cast<int>(ShaderType::BlinnPhong));
            ImGui::RadioButton("Cook-Torrance shader", &nextShader, static_cast<int>(ShaderType::CookTorrance));
            ImGui::RadioButton("Cel shader", &nextShader, static_cast<int>(ShaderType::Cel));
            ImGui::RadioButton("Gooch shader", &nextShader, static_cast<int>(ShaderType::Gooch));

            ShaderType selectedType = static_cast<ShaderType>(nextShader);
            if (selectedType != ShaderType::Normal)
            {
                ImGui::Indent();
                if (ImGui::CollapsingHeader("Material controls"))
                {
                    ImGui::Indent();

                    ImGui::ColorEdit3("Ambient color", material.m_Ambient.data());
                    ImGui::ColorEdit3("Diffuse color", material.m_Diffuse.data());
                    ImGui::ColorEdit3("Specular color", material.m_Specular.data());
                    if (selectedType != ShaderType::CookTorrance)
                        ImGui::SliderFloat("Shine constant", &material.m_Shine, 10, 100);

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

                if (selectedType == ShaderType::Gooch)
                {
                    if (ImGui::CollapsingHeader("Gooch controls"))
                    {
                        ImGui::Indent();

                        ImGui::ColorEdit3("Warm color", gooch.warm);
                        ImGui::ColorEdit3("Cool color", gooch.cool);
                        ImGui::SliderFloat("Alpha##gooch", &gooch.alpha, 0, 1);
                        ImGui::SliderFloat("Beta", &gooch.beta, 0, 1);

                        ImGui::Spacing();

                        ImGui::Unindent();
                    }
                }

                if (selectedType == ShaderType::CookTorrance)
                {
                    if (ImGui::CollapsingHeader("Cook-Torrance controls"))
                    {
                        ImGui::Indent();

                        ImGui::SliderFloat("Metallic", &pbr.metallic, 0, 1);
                        ImGui::SliderFloat("Roughness", &pbr.roughness, 0.05f, 1);

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
                ImGui::Text("Number of polygons: %u", numFaces);
                for (const auto& [size, count] : mesh.m_NumPolygons)
                    ImGui::Text("Number of %u-gons: %u", size, count);
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
            shaders.SetActiveShader(static_cast<ShaderType>(currShader));
        }

        ////////// upload uniforms //////////
        renderer.SetModelMatrix(modelMatrix);

        ////////// regenerate object //////////
        if (nextShadingType != currShadingType)
        {
            currShadingType = nextShadingType;

            mesh.Rebuild(currShadingType); // rebuild mesh based on shading type

            renderer.SetMesh(mesh);
        }

        ////////// regenerate object //////////
        if (nextObject != currObject)
        {
            currObject = nextObject;

            mesh = objects.findMesh(currObject);
            triCount = static_cast<int>(mesh.m_TriFaceIndices.size()); desiredTriCount = triCount;

            ModifyModel = true;
        }

        ////////// apply modification //////////
        if (ModifyModel)
        {
            mesh.m_ShadingType = currShadingType;
            mesh.Rebuild(); // rebuild mesh based on object info
            numFaces = static_cast<unsigned int>(mesh.m_FaceIndices.size());

            triCount = static_cast<int>(mesh.m_TriFaceIndices.size()); desiredTriCount = triCount;

            renderer.SetMesh(mesh);
            ModifyModel = false;
        }

        ////////// change render mode //////////
        if (nextRenderMode != currRenderMode)
        {
            currRenderMode = nextRenderMode;

            if (currRenderMode == POLYGON)
                renderer.SetRenderMode(RenderMode::Polygon);
            else if (currRenderMode == WIREFRAME)
                renderer.SetRenderMode(RenderMode::Wireframe);
            else if (currRenderMode == POINTCLOUD)
                renderer.SetRenderMode(RenderMode::PointCloud);
        }

        ////////// Render object here //////////
        renderer.Draw();

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
