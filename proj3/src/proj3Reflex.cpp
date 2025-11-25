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
 * @brief Project 2: Mixed Scenes (Table + Solar System) with reflection plane
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
#define sgA scene::graph()->buildAt("sgA_root") // will contain cube + sphere
#define sgB scene::graph()->buildAt("sgB_root") // will contain a plane
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
            for (auto& c : m_controllers) {
                if (!c) continue;
                c->updateOrbit(xpos, ypos);
                c->updatePan(xpos, ypos);
            }
        }

        void handleScroll(GLFWwindow* window, double xoffset, double yoffset) override {
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
        std::cout << "=== Project 2: Mixed Scenes (with Reflection Plane) ===" << std::endl;
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
            phong_shader->configureStaticUniform<float>("u_reflectionFactor", 
                []() { return 0.4f; });
            material::stack()->defineDefault("u_material_alpha", 1.0f); // Default alpha uniform
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
        scene::graph()->addNode("sgA_root");
        scene::graph()->addNode("sgB_root");

        // Build sgA: cube and sphere
        {
            sgA.addNode("sgA_cube")
                .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->translate(-1.5f, 0.0f, 0.0f)
                    ->scale(1.0f,1.0f,1.0f)
            )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.8f,0.2f,0.2f))
                )
                .with<component::GeometryComponent>(cube_geom);

            sgA.addNode("sgA_sphere")
                .with<component::TransformComponent>(
                transform::Transform::Make()->translate(1.5f, 0.0f, 0.0f)->scale(0.8f,0.8f,0.8f)
            )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.2f,0.2f,0.8f))
                )
                .with<component::GeometryComponent>(sphere_geom);
            
            // Add pink sphere at origin
            sgA.addNode("sgA_pink_sphere")
                .with<component::TransformComponent>(
                transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f)->scale(0.6f,0.6f,0.6f)
            )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(1.0f, 0.4f, 0.7f))
                )
                .with<component::GeometryComponent>(sphere_geom);

            // Create a target node at origin for camera to look at
            sgA.addNode("sgA_camera_target")
            .with<component::ObservedTransformComponent>(
                transform::Transform::Make(),
                "origin_target"
            );

            // Add a perspective camera to sgA and set it active (will become global camera)
            auto camComp = component::PerspectiveCamera::Make(60.0f, 1.0f, 100.0f);
            auto camNode = sgA.addNode("sgA_cam")
                .addComponent(camComp);
            // Position the camera back so objects at origin are in view
            camComp->getTransform()->translate(0.0f, 1.0f, 6.0f);
            auto targetTransform = scene::graph()->getNodeByName("sgA_camera_target")
                ->payload()
                .get<component::ObservedTransformComponent>("origin_target");
            // Make camera look at the target at origin
            camComp->setTarget(targetTransform);
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

            // alternative:
            // light::PointLight::Make(
            //             {
            //                 .position = glm::vec4(2.0f, 3.0f, 2.0f, 1.0f),
            //                 .ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f),
            //                 .diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            //                 .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            //                 .constant = 1.0f,
            //                 .linear = 0.09f,
            //                 .quadratic = 0.032f
            //             }
            //         )

            auto lightNode = sgA.addNode("sgA_point_light")
                .with<component::LightComponent>(
                    point_light,
                    transform::Transform::Make());
            // Update light manager so the GPU UBO receives the new light data
            light::manager().apply();
        }

        // Build sgB: a single plane
        {
            sgB.addNode("sgB_plane")
                .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->translate(0.0f, -2.0f, 0.0f)
                    ->scale(4.0f,0.1f,4.0f)
                )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.4f,0.8f,0.4f))
                        ->setDiffuse(glm::vec4(0.4f, 0.8f, 0.4f, 0.5f)) // Semi-transparent
                        ->set<float>("u_material_alpha", 0.5f)
                )
                .with<component::GeometryComponent>(cube_geom);
        }

        // Reflection plane and draw function removed (reflection pass disabled)


        // 6. Common Components
        auto phong_shader_comp = component::ShaderComponent::Make(phong_shader);
        auto no_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");

        // TABLE SCENE removed per user request.
        // Create arcball controllers from the actual camera nodes in each SceneGraph
        auto sgA_cam_node = scene::graph()->getNodeByName("sgA_cam");
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

    // ==================================================================================
    // PRE-CONFIGURATION OF RENDER STATES
    // ==================================================================================
    
    // State 1: Stencil Masking (Write 1s to stencil buffer where the plane is)
    auto maskState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        s->stencil().setTest(true);
        // Equivalent to: glStencilFunc(GL_NEVER, 1, 0xFFFF)
        s->stencil().setFunction(framebuffer::StencilFunc::Never, 1, 0xFFFF);
        // Equivalent to: glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE)
        s->stencil().setOperation(framebuffer::StencilOp::Replace, 
                                framebuffer::StencilOp::Replace, 
                                framebuffer::StencilOp::Replace);
        return s;
    }();

    // State 2: Reflection Drawing (Draw only where stencil == 1)
    auto reflectionState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        s->stencil().setTest(true);
        // Equivalent to: glStencilFunc(GL_EQUAL, 1, 0xFFFF)
        s->stencil().setFunction(framebuffer::StencilFunc::Equal, 1, 0xFFFF);
        // Equivalent to: glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP)
        s->stencil().setOperation(framebuffer::StencilOp::Keep, 
                                framebuffer::StencilOp::Keep, 
                                framebuffer::StencilOp::Keep);
        return s;
    }();

    // State 3: Blending (For the semi-transparent mirror surface)
    auto blendState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        s->blend().setEnabled(true);
        // Equivalent to: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        s->blend().setFunction(framebuffer::BlendFactor::SrcAlpha, 
                            framebuffer::BlendFactor::OneMinusSrcAlpha);
        return s;
    }();

    // Render: draw both scene graphs (sgA, sgB)
    auto on_render = [&](double alpha) {
        // 0. Clear buffers (Immediate operation)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // ==================================================================================
        // RENDER PASSES
        // ==================================================================================

        // Step 1: Mark reflector plane
        // Push nullptr (Default Framebuffer) with maskState
        framebuffer::stack()->push(nullptr, maskState); 
        {
            scene::graph()->drawSubtree("sgB_root");
        }
        framebuffer::stack()->pop(); // Automatically restores stencil state to default (Disabled)

        
        // Step 2: Draw reflected scene
        // Push nullptr with reflectionState
        framebuffer::stack()->push(nullptr, reflectionState);
        {
            // Matrix math
            // Apply reflection transformation across plane y = -2.0
            // We mirror objects through the plane at y = -2
            glm::mat4 reflectionMatrix = glm::mat4(1.0f);
            reflectionMatrix[1][1] = -1.0f;  // Scale Y by -1
            reflectionMatrix[3][1] = -4.0f;  // Translate Y by -4 (2 * planeY where planeY = -2)
            
            transform::stack()->push(reflectionMatrix);
            
            // Note: glFrontFace is Rasterizer state, not FBO state, 
            // so it remains a raw call as it is not covered by framebuffer.h
            // With negative scale on Y, winding order is reversed
            glFrontFace(GL_CW); 
            scene::graph()->drawSubtree("sgA_root");
            glFrontFace(GL_CCW);
            
            transform::stack()->pop();
        }
        framebuffer::stack()->pop(); // Automatically restores stencil state to default (Disabled)


        // Step 3: Draw normal scene
        // No state push needed (uses default state from base of stack)
        scene::graph()->drawSubtree("sgA_root");


        // Step 4: Draw reflector plane with blending
        // Push nullptr with blendState
        framebuffer::stack()->push(nullptr, blendState);
        {
            scene::graph()->drawSubtree("sgB_root");
        }
        framebuffer::stack()->pop(); // Automatically restores blend state to default (Disabled)

        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Project 2: Mixed Scenes (Table + Solar System) - Reflection";
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
