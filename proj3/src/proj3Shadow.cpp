// proj3.cpp - versão final com plano de reflexão
#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <gl_base/texture.h>
#include <gl_base/cubemap.h>
#include <3d/camera/perspective_camera.h>
#include <3d/lights/directional_light.h>
#include <3d/lights/point_light.h>
#include <3d/lights/spot_light.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/3d_shapes/cylinder.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <gl_base/uniforms/uniform.h>
#include <other_genes/environment_mapping.h>

// Reflection pass removed per user request

/**
 * @brief Project 2: Mixed Scenes (Table + Solar System) with planar shadow
 *
 * Controls:
 * - '1' Key: Switch to Table Scene
 * - '2' Key: Switch to Solar System Scene
 * - 'C' Key: Switch cameras (Solar System only)
 * - Left Mouse Button + Drag: Rotate camera (orbit)
 * - Middle Mouse Button + Drag: Pan camera
 * - Mouse Wheel: Zoom in/out
 * - ESC: Exit
 */

// ---------- Globals ----------
std::shared_ptr<arcball::ArcBallController> table_arcball;
std::shared_ptr<arcball::ArcBallController> sgB_arcball;

shader::ShaderPtr phong_shader;
shader::ShaderPtr emissive_shader;
shader::ShaderPtr reflection_shader;

// Reflection framebuffer
framebuffer::FramebufferPtr reflectionFBO;
const int REFLECTION_WIDTH = 1024;
const int REFLECTION_HEIGHT = 1024;

// Textures
texture::TexturePtr woodTexture;
texture::TexturePtr woodNormalMap;
texture::TexturePtr basketballTexture;
texture::TexturePtr basketballNormalMap;
texture::TexturePtr roughnessTexture;

// Cameras
std::shared_ptr<component::PerspectiveCamera> table_cam;

// Reflection-related data removed

// Additional SceneGraphs
scene::SceneGraphPtr sgA; // will contain cube + sphere
scene::SceneGraphPtr sgB; // will contain a plane
int main() {
    // Custom input handler that forwards mouse/scroll events to multiple ArcBall controllers
    class MultiArcballHandler : public input::InputHandler {
    public:
        void addController(const std::shared_ptr<arcball::ArcBallController>& c) {
            if (c) m_controllers.push_back(c);
        }

    protected:
        void handleMouseButton(GLFWwindow* window, int button, int action, int mods) override {
            // Debug: log mouse button events
            double dbg_mx = 0.0, dbg_my = 0.0;
            glfwGetCursorPos(window, &dbg_mx, &dbg_my);
            std::cout << "[Input Debug] MouseButton button=" << button
                      << " action=" << action
                      << " pos=(" << dbg_mx << "," << dbg_my << ")"
                      << " controllers=" << m_controllers.size() << std::endl;

            for (auto& c : m_controllers) {
                if (!c) continue;
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    if (action == GLFW_PRESS) c->startOrbit(mx, my);
                    else if (action == GLFW_RELEASE) c->endOrbit();
                } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                    if (action == GLFW_PRESS) c->startPan(mx, my);
                    else if (action == GLFW_RELEASE) c->endPan();
                }
            }
        }

        void handleCursorPos(GLFWwindow* window, double xpos, double ypos) override {
            // Only print cursor positions when a mouse button is pressed to reduce spam
            int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            int midState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
            if (leftState == GLFW_PRESS || midState == GLFW_PRESS) {
                std::cout << "[Input Debug] CursorPos xpos=" << xpos << " ypos=" << ypos
                          << " (controllers=" << m_controllers.size() << ")" << std::endl;
            }

            for (auto& c : m_controllers) {
                if (!c) continue;
                c->updateOrbit(xpos, ypos);
                c->updatePan(xpos, ypos);
            }
        }

        void handleScroll(GLFWwindow* window, double xoffset, double yoffset) override {
            std::cout << "[Input Debug] Scroll x=" << xoffset << " y=" << yoffset
                      << " controllers=" << m_controllers.size() << std::endl;
            for (auto& c : m_controllers) {
                if (!c) continue;
                c->zoom(yoffset);
            }
        }

    private:
        std::vector<std::shared_ptr<arcball::ArcBallController>> m_controllers;
    };

    auto* handler = new MultiArcballHandler();

    auto on_init = [&](engene::EnGene& app) {
        std::cout << "=== Project 2: Mixed Scenes (with Planar Shadow) ===" << std::endl;
        std::cout << "Press '1' for Table Scene, '2' for Solar System" << std::endl;

        // 1. Create Lights First (so SceneLights UBO exists before shader compilation)
        std::cout << "[INIT] Creating lights..." << std::endl;
        
        // Dummy lights that will be replaced by scene-specific lights
        light::DirectionalLightParams dummy_dir;
        dummy_dir.base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        dummy_dir.ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        dummy_dir.diffuse = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        dummy_dir.specular = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        auto dummy_light = light::DirectionalLight::Make(dummy_dir);
        scene::graph()->addNode("dummy_light")
            .with<component::LightComponent>(dummy_light, transform::Transform::Make());
        
        light::manager().apply();
        std::cout << "✓ Initial lights created" << std::endl;

        // 2. Load Shaders
        try {
            phong_shader = shader::Shader::Make("shaders/phong.vert", "shaders/phong.frag");
            phong_shader->addResourceBlockToBind("CameraMatrices");
            phong_shader->addResourceBlockToBind("CameraPosition");
            phong_shader->addResourceBlockToBind("SceneLights");
            phong_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            phong_shader->configureDynamicUniform<glm::mat4>("u_projectorViewProj", 
                []() { return glm::mat4(1.0f); }); // Dummy projector matrix
            phong_shader->configureDynamicUniform<float>("u_reflectionFactor", 
                []() { return 0.4f; });
            // Add uniform to control shadow rendering mode
            phong_shader->configureStaticUniform<bool>("u_renderShadow", []() { return false; });
            material::stack()->configureShaderDefaults(phong_shader);
            phong_shader->Bake();

            emissive_shader = shader::Shader::Make("shaders/sun.vert", "shaders/sun.frag");
            emissive_shader->addResourceBlockToBind("CameraMatrices");
            emissive_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            material::stack()->configureShaderDefaults(emissive_shader);
            emissive_shader->Bake();

            // Reflection shader for the plane
            reflection_shader = shader::Shader::Make("shaders/reflection.vert", "shaders/reflection.frag");
            reflection_shader->addResourceBlockToBind("CameraMatrices");
            reflection_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            // Configure sampler uniform using the engine's sampler provider (type-safe)
            reflection_shader->configureDynamicUniform<uniform::detail::Sampler>(
                "u_reflectionTexture", texture::getSamplerProvider("u_reflectionTexture"));
            // Configure a default tint so the reflection plane is visually distinct from the background
            reflection_shader->configureStaticUniform<glm::vec3>("u_tintColor", []() { return glm::vec3(0.08f, 0.15f, 0.35f); });
            reflection_shader->configureStaticUniform<float>("u_tintFactor", []() { return 0.25f; });
            reflection_shader->Bake();
            std::cout << "✓ Reflection shader configured" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Shader compilation failed: " << e.what() << std::endl;
            throw;
        }

        // Create reflection framebuffer using engine's Framebuffer class
        reflectionFBO = framebuffer::Framebuffer::MakeRenderToTexture(
            REFLECTION_WIDTH, REFLECTION_HEIGHT,
            "reflectionColor",  // texture name
            framebuffer::attachment::Format::RGB8,
            framebuffer::attachment::Format::DepthComponent24
        );
       
        // Table scene textures with normal maps
        woodTexture = texture::Texture::Make("assets/images/table-tex.jpg");
        woodNormalMap = texture::Texture::Make("assets/images/table-normal.jpg");
        basketballTexture = texture::Texture::Make("assets/images/basketball-tex.png");
        basketballNormalMap = texture::Texture::Make("assets/images/basketball-normal.jpg");
        roughnessTexture = texture::Texture::Make("assets/images/noise.png");
        
        // 4. Create Geometries
        auto cube_geom = Cube::Make();
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);
        auto cylinder_geom = Cylinder::Make(1.0f, 1.0f, 32);

        // Create two separate scene graphs
        sgA = scene::SceneGraph::Make();
        sgB = scene::SceneGraph::Make();

        // Build sgA: cube and sphere
        {
            auto rootA = sgA->getRoot();

            auto cubeNode = sgA->addNode("sgA_cube", rootA);
            cubeNode->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(-1.5f, 0.0f, 0.0f)->scale(1.0f,1.0f,1.0f)
            ), cubeNode);
            cubeNode->payload().addComponent(component::ShaderComponent::Make(phong_shader), cubeNode);
            // Create a material and wrap it in a MaterialComponent
            auto cube_mat = material::Material::Make(glm::vec3(0.8f,0.2f,0.2f));
            cubeNode->payload().addComponent(component::MaterialComponent::Make(cube_mat), cubeNode);
            cubeNode->payload().addComponent(component::GeometryComponent::Make(cube_geom), cubeNode);

            auto sphereNode = sgA->addNode("sgA_sphere", rootA);
            sphereNode->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(1.5f, 0.0f, 0.0f)->scale(0.8f,0.8f,0.8f)
            ), sphereNode);
            sphereNode->payload().addComponent(component::ShaderComponent::Make(phong_shader), sphereNode);
            auto sphere_mat = material::Material::Make(glm::vec3(0.2f,0.2f,0.8f));
            sphereNode->payload().addComponent(component::MaterialComponent::Make(sphere_mat), sphereNode);
            sphereNode->payload().addComponent(component::GeometryComponent::Make(sphere_geom), sphereNode);

            // Add pink sphere at origin
            auto pinkSphereNode = sgA->addNode("sgA_pink_sphere", rootA);
            pinkSphereNode->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f)->scale(0.6f,0.6f,0.6f)
            ), pinkSphereNode);
            pinkSphereNode->payload().addComponent(component::ShaderComponent::Make(phong_shader), pinkSphereNode);
            auto pink_mat = material::Material::Make(glm::vec3(1.0f, 0.4f, 0.7f));
            pinkSphereNode->payload().addComponent(component::MaterialComponent::Make(pink_mat), pinkSphereNode);
            pinkSphereNode->payload().addComponent(component::GeometryComponent::Make(sphere_geom), pinkSphereNode);

            // Create a target node at origin for camera to look at
            auto targetNode = sgA->addNode("sgA_camera_target", rootA);
            auto targetTransform = component::ObservedTransformComponent::Make(
                transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f)
            );
            targetNode->payload().addComponent(targetTransform, targetNode);

            // Add a perspective camera to sgA and set it active (will become global camera)
            auto camComp = component::PerspectiveCamera::Make(60.0f, 1.0f, 100.0f);
            auto camNode = sgA->addNode("sgA_cam", rootA);
            camNode->payload().addComponent(camComp, camNode);
            // Position the camera back so objects at origin are in view
            camNode->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(0.0f, 1.0f, 6.0f)
            ), camNode);
            // Make camera look at the target at origin
            camComp->setTarget(targetTransform);
            sgA->setActiveCamera(camComp);
            // Also set the engine's global scene graph active camera so ArcBall updates affect the renderer's camera UBO
            scene::graph()->setActiveCamera(camComp);

            // Add a point light to sgA (positioned relative to sgA's root)
            light::PointLightParams pparams;
            pparams.position = glm::vec4(2.0f, 3.0f, 2.0f, 1.0f); // x,y,z,w
            pparams.ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
            pparams.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            pparams.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            pparams.constant = 1.0f;
            pparams.linear = 0.09f;
            pparams.quadratic = 0.032f;
            auto point_light = light::PointLight::Make(pparams);
            auto lightNode = sgA->addNode("sgA_point_light", rootA);
            lightNode->payload().addComponent(component::LightComponent::Make(point_light, transform::Transform::Make()), lightNode);
            // Update light manager so the GPU UBO receives the new light data
            light::manager().apply();
        }

        // Build sgB: a single plane
        {
            auto rootB = sgB->getRoot();
            auto planeNode = sgB->addNode("sgB_plane", rootB);
            planeNode->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(0.0f, -2.0f, 0.0f)->scale(4.0f,0.1f,4.0f)
            ), planeNode);
            planeNode->payload().addComponent(component::ShaderComponent::Make(phong_shader), planeNode);
            auto plane_mat = material::Material::Make(glm::vec3(0.4f,0.8f,0.4f));
            plane_mat->setDiffuse(glm::vec4(0.4f, 0.8f, 0.4f, 0.5f)); // Semi-transparent
            planeNode->payload().addComponent(component::MaterialComponent::Make(plane_mat), planeNode);
            
            // Add alpha uniform for transparency
            auto plane_alpha = component::VariableComponent::Make(
                uniform::Uniform<float>::Make("u_material_alpha", [](){ return 0.5f; })
            );
            planeNode->payload().addComponent(plane_alpha, planeNode);
            
            planeNode->payload().addComponent(component::GeometryComponent::Make(cube_geom), planeNode);

            // Add camera for sgB (optional) but do not activate it globally
            auto camCompB = component::OrthographicCamera::Make();
            auto camNodeB = sgB->addNode("sgB_cam", rootB);
            camNodeB->payload().addComponent(camCompB, camNodeB);
            // Give sgB's camera a reasonable starting position as well
            camNodeB->payload().addComponent(component::TransformComponent::Make(
                transform::Transform::Make()->translate(0.0f, 2.0f, 8.0f)
            ), camNodeB);
        }

        // Reflection plane and draw function removed (reflection pass disabled)

        // 5. Create Materials
        auto table_material = material::Material::Make(glm::vec3(0.6f, 0.6f, 0.6f));
        table_material->setShininess(320.0f);
        table_material->setSpecular(glm::vec3(1.0f, 1.0f, 1.0f));

        auto leg_material = material::Material::Make(glm::vec3(0.6f, 0.3f, 0.1f));
        leg_material->setShininess(16.0f);

        auto sphere_material = material::Material::Make(glm::vec3(1.0f, 1.0f, 1.0f));
        sphere_material->setShininess(128.0f);
        sphere_material->setSpecular(glm::vec3(1.0f, 1.0f, 1.0f));

        auto lamp_material = material::Material::Make(glm::vec3(0.1f, 0.4f, 0.8f));
        lamp_material->setShininess(64.0f);
        lamp_material->setSpecular(glm::vec3(1.0f, 1.0f, 1.0f));


        // 6. Common Components
        auto phong_shader_comp = component::ShaderComponent::Make(phong_shader);
        auto no_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");

        // TABLE SCENE removed per user request.
        // Create arcball controllers from the actual camera nodes in each SceneGraph
        auto sgA_cam_node = sgA->getNodeByName("sgA_cam");
        if (sgA_cam_node) {
            table_arcball = arcball::ArcBallController::CreateFromCameraNode(sgA_cam_node);
            table_arcball->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            table_arcball->setZoomLimits(2.0f, 20.0f);
            table_arcball->setSensitivity(0.005f, 1.0f, 0.001f);
            static_cast<MultiArcballHandler*>(handler)->addController(table_arcball);
        } else {
            std::cerr << "ArcBall: sgA camera node not found" << std::endl;
        }

        // We only create one ArcBall controller for the active camera (sgA).
        // The global camera is shared by both scene graphs, so rotating it will affect both.

        handler->registerCallback<input::InputType::KEY>([&](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (action != GLFW_PRESS) return;
        });

        // ========== INIT REFLECTION PASS ==========
        // Reflection pass removed per user request.

        std::cout << "✓ Scenes initialized." << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  '1' - Table Scene" << std::endl;
        std::cout << "  '2' - Solar System Scene" << std::endl;
        std::cout << "  'C' - Switch cameras (Solar System only)" << std::endl;
    };

    auto on_update = [&](double dt) {
        // update logic (se precisar animar algo)
    };

    // Render: draw both scene graphs (sgA, sgB)
    auto on_render = [&](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Render based on display mode
            // Step 1: Draw scene without shadow receivers (just sgA)
            if (sgA) sgA->draw();
            
            // Step 2: Draw shadow receiver with ambient light only
            // TODO: Need ambient-only rendering mode for sgB
            // For now, draw the plane with full lighting
            if (sgB) sgB->draw();
            
            // Step 3: For each light source, mark shadows in stencil and illuminate
            // Light position (point light from sgA)
            glm::vec4 lightPos = glm::vec4(2.0f, 3.0f, 2.0f, 1.0f);
            glm::vec4 planeNormal = glm::vec4(0.0f, 1.0f, 0.0f, -2.0f); // Normal (0,1,0) and d=-2 for plane y=-2
            
            // Step 3a: Mark shadows in stencil buffer
            glEnable(GL_STENCIL_TEST);
            glStencilFunc ( GL_NEVER , 1 , 0xFFFF );
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

            // Create shadow projection matrix using correct formula
            auto shadowMatrix = [](const glm::vec4& n, const glm::vec4& l) -> glm::mat4 {
                float ndotl = glm::dot(glm::vec3(n), glm::vec3(l));
                return glm::mat4(
                    glm::vec4(ndotl + n.w - l.x * n.x, -l.y * n.x, -l.z * n.x, -n.x),
                    glm::vec4(-l.x * n.y, ndotl + n.w - l.y * n.y, -l.z * n.y, -n.y),
                    glm::vec4(-l.x * n.z, -l.y * n.z, ndotl + n.w - l.z * n.z, -n.z),
                    glm::vec4(-l.x * n.w, -l.y * n.w, -l.z * n.w, ndotl)
                );
            };
            
            glm::mat4 shadowProj = shadowMatrix(planeNormal, lightPos);
            
            // Apply shadow matrix to u_model via transform stack
            // This will make u_model = shadowProj * originalModel for all objects
            transform::stack()->push(shadowProj);
            
            // Draw scene to mark shadow volumes in stencil
            if (sgA) sgA->draw();
            
            transform::stack()->pop();
            
            // Re-enable color writes
            
            // Step 3b: Illuminate receiver where stencil is NOT marked (no shadow)
            // Draw receiver again with lighting, only where stencil == 0
            glStencilFunc(GL_EQUAL, 0, 0xFFFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glBlendFunc(GL_ONE, GL_ONE);  // Additive blending for lighting
            glEnable(GL_BLEND);
            glDepthFunc(GL_EQUAL);
            
            if (sgB) sgB->draw();
            glDepthFunc(GL_LESS);
            glDisable(GL_BLEND);
            glDisable(GL_STENCIL_TEST);

        GL_CHECK("render");
        
    };

    engene::EnGeneConfig config;
    config.title = "Project 2: Mixed Scenes (Table + Solar System) - Planar Shadow";
    config.width = 1280;
    config.height = 720;
    config.clearColor[0] = 1.0f;
    config.clearColor[1] = 1.0f;
    config.clearColor[2] = 1.0f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config, handler);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
