// proj3Shadow.cpp - Final version with Planar Shadow
#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <gl_base/texture.h>
#include <gl_base/cubemap.h>
#include <gl_base/framebuffer.h>
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
std::shared_ptr<arcball::ArcBallController> scene_arcball;

// Camera and light
component::PerspectiveCameraPtr camComp;
component::LightComponentPtr point_light_comp;
light::PointLightPtr point_light;


shader::ShaderPtr phong_shader;

// Reflection framebuffer
framebuffer::FramebufferPtr shadowFBO;

// Helper macros for SceneBuilder syntax (matches reference style)
#define sgA scene::graph()->buildAt("sgA_root")
#define sgB scene::graph()->buildAt("sgB_root")

int main() {
    // Custom input handler that forwards mouse/scroll events to multiple ArcBall controllers
    class MultiArcballHandler : public input::InputHandler {
    public:
        void addController(const std::shared_ptr<arcball::ArcBallController>& c) {
            if (c) m_controllers.push_back(c);
        }

    protected:
        void handleMouseButton(GLFWwindow* window, int button, int action, int mods) override {
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
        light::DirectionalLightParams dummy_dir;
        dummy_dir.base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        dummy_dir.ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        dummy_dir.diffuse = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        dummy_dir.specular = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        auto dummy_light = light::DirectionalLight::Make(dummy_dir);
        scene::graph()->addNode("dummy_light")
            .with<component::LightComponent>(dummy_light, transform::Transform::Make());
        
        light::manager().apply();

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
            // Add uniform to control shadow rendering mode
            phong_shader->configureStaticUniform<bool>("u_renderShadow", []() { return false; });
            material::stack()->configureShaderDefaults(phong_shader);
            phong_shader->Bake();
        } catch (const std::exception& e) {
            std::cerr << "Shader compilation failed: " << e.what() << std::endl;
            throw;
        }

        // 1. Create the Shadow FBO
        // We use a high resolution (2048x2048) for sharper shadows.
        // MakeShadowMap automatically configures GL_COMPARE_REF_TO_TEXTURE for hardware PCF.
        auto shadowFBO = framebuffer::Framebuffer::MakeShadowMap(
            2048, 2048, 
            "shadow_depth_map", 
            framebuffer::attachment::Format::DepthComponent24
        );

        // 2. Create RenderState for the Shadow Pass
        // We often want to cull FRONT faces during the shadow pass to fix "peter panning" artifacts.
        auto shadowPassState = [](){
            auto s = std::make_shared<framebuffer::RenderState>();
            s->depth().setTest(true);
            s->depth().setWrite(true);
            s->depth().setFunction(framebuffer::DepthFunc::Less);
            
            // Optional: If your engine supports setting cull face in RenderState, do it here.
            // If not, you might need to handle glCullFace manually or add it to RenderState.
            // For now, we rely on standard depth testing.
            return s;
        }();
        
        // 4. Create Geometries
        auto cube_geom = Cube::Make();
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);

        // Create separate scene graph roots
        scene::graph()->addNode("sgA_root");
        scene::graph()->addNode("sgB_root");

        // Build sgA: cube and sphere
        {
            sgA.addNode("sgA_cube")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(-1.5f, 0.0f, 0.0f)->scale(1.0f,1.0f,1.0f)
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
                    transform::Transform::Make()
                    ->translate(0.0f, 0.0f, 0.0f),
                    "origin_target"
                );

            // Add a perspective camera to sgA
            camComp = component::PerspectiveCamera::Make(60.0f, 1.0f, 100.0f);
            sgA.addNode("sgA_cam").addComponent(camComp);
            
            // Position the camera
            camComp->getTransform()->translate(0.0f, 1.0f, 6.0f);
            auto targetTransform = scene::graph()->getNodeByName("sgA_camera_target")
                ->payload().get<component::ObservedTransformComponent>("origin_target");
            
            camComp->setTarget(targetTransform);
            scene::graph()->setActiveCamera(camComp);

            // Add a point light to sgA (positioned relative to sgA's root)
            light::PointLightParams pparams;
            pparams.position = glm::vec4(2.0f, 3.0f, 2.0f, 1.0f);
            pparams.ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
            pparams.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            pparams.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            pparams.constant = 1.0f;
            pparams.linear = 0.09f;
            pparams.quadratic = 0.032f;
            point_light = light::PointLight::Make(pparams);
            point_light_comp = component::LightComponent::Make(point_light, transform::Transform::Make());
            
            sgA.addNode("sgA_point_light")
                .addComponent(point_light_comp);
                
            // Update light manager so the GPU UBO receives the new light data
            light::manager().apply();
        }

        // Build sgB: a single plane
        {
            sgB.addNode("sgB_plane")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, -2.0f, 0.0f)->scale(40.0f,0.1f,40.0f)
                )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.4f,0.8f,0.4f))
                        ->setDiffuse(glm::vec4(0.4f, 0.8f, 0.4f, 0.5f)) // Semi-transparent
                )
                .with<component::VariableComponent>(
                    uniform::Uniform<float>::Make("u_material_alpha", [](){ return 0.5f; })
                )
                .with<component::GeometryComponent>(cube_geom);
        }

        // Create arcball controllers
        auto sgA_cam_node = scene::graph()->getNodeByName("sgA_cam");
        if (sgA_cam_node) {
            scene_arcball = arcball::ArcBallController::CreateFromCameraNode(sgA_cam_node);
            scene_arcball->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            scene_arcball->setZoomLimits(2.0f, 20.0f);
            scene_arcball->setSensitivity(0.005f, 1.0f, 0.001f);
            static_cast<MultiArcballHandler*>(handler)->addController(scene_arcball);
        } else {
            std::cerr << "ArcBall: sgA camera node not found" << std::endl;
        }

        handler->registerCallback<input::InputType::KEY>([&](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (action != GLFW_PRESS) return;
        });
    };

    auto on_update = [&](double dt) {
        // update logic
    };

    // ==================================================================================
    // PRE-CONFIGURATION OF RENDER STATES
    // ==================================================================================

    // State 1: Mark Shadows (Write to Stencil)
    auto markShadowState = [](){
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

    // State 2: Illuminate Receivers (Blend + Depth Equal + Stencil 0)
    auto illuminateState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        
        // Stencil: Only draw where stencil == 0 (no shadow)
        s->stencil().setTest(true);
        s->stencil().setFunction(framebuffer::StencilFunc::Equal, 0, 0xFFFF);
        s->stencil().setOperation(framebuffer::StencilOp::Keep, 
                                  framebuffer::StencilOp::Keep, 
                                  framebuffer::StencilOp::Keep);
        
        // Blend: Additive (One, One)
        s->blend().setEnabled(true);
        s->blend().setFunction(framebuffer::BlendFactor::One, framebuffer::BlendFactor::One);
        
        // Depth: Equal (to draw exactly on top of the ambient pass)
        s->depth().setFunction(framebuffer::DepthFunc::Equal);
        
        return s;
    }();

    // Render: draw both scene graphs
    auto on_render = [&](double alpha) {
        // 0. Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Step 1: Draw scene without shadow receivers (sgA)
        scene::graph()->drawSubtree("sgA_root");
        
        // Step 2: Draw shadow receiver with ambient light only
        // TODO: Need ambient-only rendering mode for sgB
        // For now, draw the plane with full lighting
        scene::graph()->drawSubtree("sgB_root");
        
        // Step 3: For each light source, mark shadows in stencil and illuminate
        glm::vec4 lightPos = point_light_comp->getWorldTransform() * point_light->getPosition();
        glm::vec4 planeNormal = glm::vec4(0.0f, 1.0f, 0.0f, 2.0f); // Normal (0,1,0) and d=-2 for plane y=-2
        
        // Step 3a: Mark shadows in stencil buffer
        // Push nullptr (Default FBO) with markShadowState
        framebuffer::stack()->push(nullptr, markShadowState);
        {
            // Create shadow projection matrix
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
            transform::stack()->push(shadowProj);
            
            // Draw scene to mark shadow volumes in stencil
            scene::graph()->drawSubtree("sgA_root");
            
            transform::stack()->pop();
        }
        framebuffer::stack()->pop(); // Restore default state
        
        // Step 3b: Illuminate receiver where stencil is NOT marked (no shadow)
        // Push nullptr with illuminateState
        framebuffer::stack()->push(nullptr, illuminateState);
        {
            scene::graph()->drawSubtree("sgB_root");
        }
        framebuffer::stack()->pop(); // Restore default state

        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Project 3: Reflections and Shadows";
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