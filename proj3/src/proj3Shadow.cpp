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

// Shaders
shader::ShaderPtr phong_shader;
shader::ShaderPtr shadow_shader;

// Shared resources
glm::mat4 light_view_proj_matrix;

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
            phong_shader->configureDynamicUniform<glm::mat4>("u_lightSpaceMatrix", 
                []() { return light_view_proj_matrix; });
            phong_shader->configureStaticUniform<float>("u_shadowBias", 
                []() { return 0.005f; });
            material::stack()->configureShaderDefaults(phong_shader);
            phong_shader->Bake();

            shadow_shader = shader::Shader::Make("shaders/shadow.vert", "shaders/shadow.frag");
            shadow_shader->addResourceBlockToBind("CameraMatrices");
            shadow_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            shadow_shader->configureStaticUniform<float>("u_shadowBias", 
                []() { return 0.005f; });
            shadow_shader->configureStaticUniform<glm::mat4>("u_lightViewProj", []() { return light_view_proj_matrix; });
            shadow_shader->Bake();


        } catch (const std::exception& e) {
            std::cerr << "Shader compilation failed: " << e.what() << std::endl;
            throw;
        }

        // 1. Create the Shadow FBO
        // We use a high resolution (2048x2048) for sharper shadows.
        // MakeShadowMap automatically configures GL_COMPARE_REF_TO_TEXTURE for hardware PCF.
        shadowFBO = framebuffer::Framebuffer::MakeShadowMap(
            2048, 2048, 
            "shadow_depth_map", 
            framebuffer::attachment::Format::DepthComponent24
        );

        shadowFBO->attachToShader(shadow_shader, {{"shadow_depth_map", "u_shadowMap"}});
        
        // 4. Create Geometries
        auto cube_geom = Cube::Make();
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);

        // Create separate scene graph roots
        scene::graph()->addNode("sgA_root")
            .with<component::ShaderComponent>(phong_shader)
            .with<component::TextureComponent>(
                shadowFBO->getTexture("shadow_depth_map"),
                "u_shadowMap", 1  // Bind shadow map to texture unit 1
            );
        scene::graph()->addNode("sgB_root")
            .with<component::ShaderComponent>(phong_shader)
            .with<component::TextureComponent>(
                shadowFBO->getTexture("shadow_depth_map"),
                "u_shadowMap", 1  // Bind shadow map to texture unit 1
            );

        // Build sgA: cube and sphere
        {
            sgA.addNode("sgA_cube")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(-1.5f, 0.0f, 0.0f)->scale(1.0f,1.0f,1.0f)
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.8f,0.2f,0.2f))
                )
                .with<component::GeometryComponent>(cube_geom);

            sgA.addNode("sgA_sphere")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(1.5f, 0.0f, 0.0f)->scale(0.8f,0.8f,0.8f)
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.2f,0.2f,0.8f))
                )
                .with<component::GeometryComponent>(sphere_geom);

            // Add pink sphere at origin
            sgA.addNode("sgA_pink_sphere")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f)->scale(0.6f,0.6f,0.6f)
                )
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

    // Render: draw both scene graphs
    auto on_render = [&](double alpha) {
        // =============================================================
        // CALCULATE LIGHT MATRICES
        // =============================================================
        glm::vec3 lightPos = glm::vec3(point_light_comp->getWorldTransform() * point_light->getPosition());
        
        // Define the view frustum for the light
        // For a directional light, use glm::ortho. For point/spot, use glm::perspective.
        float near_plane = 0.1f, far_plane = 20.0f;
        glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 
                                            (float)shadowFBO->getWidth() / shadowFBO->getHeight(), 
                                            near_plane, far_plane);
        
        // Look at the center of the scene (or wherever sgA is)
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        light_view_proj_matrix = lightProj * lightView;

        // =============================================================
        // PASS 1: SHADOW MAP GENERATION
        // =============================================================
        
        // Push the Shadow FBO and the Shadow State
        framebuffer::stack()->push(shadowFBO, shadowPassState);
        {
            // Clear ONLY the depth buffer (Shadow maps don't have color)
            glClear(GL_DEPTH_BUFFER_BIT);

            // Configure shader to use Light's ViewProjection
            // Assuming your shader system has a way to set the view-proj matrix:
            scene::graph()->getNodeByName("sgA_root")
                ->payload().get<component::ShaderComponent>()
                ->setShader(shadow_shader);

            // We only render objects that CAST shadows (sgA)
            // Note: Render face culling (Front Face Culling) is recommended here 
            // to prevent self-shadowing acne, usually handled via glCullFace(GL_FRONT).
            scene::graph()->drawSubtree("sgA_root");
        }
        framebuffer::stack()->pop(); // Restore Default FBO and previous State
        scene::graph()->getNodeByName("sgA_root")
            ->payload().get<component::ShaderComponent>()
            ->setShader(phong_shader); // Restore original shader

        // =============================================================
        // PASS 2: LIGHTING / SCENE RENDERING
        // =============================================================

        // Clear the default framebuffer (Screen)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 3. Draw the Scene
        // Draw the casters (sgA)
        scene::graph()->drawSubtree("sgA_root");
        
        // Draw the receivers (sgB / Plane)
        scene::graph()->drawSubtree("sgB_root");

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