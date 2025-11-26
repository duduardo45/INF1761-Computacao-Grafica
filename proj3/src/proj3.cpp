// proj3.cpp - Combined version with Planar Reflection and Shadow
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
 * @brief Project 3: Reflections and Shadows
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
std::shared_ptr<arcball::ArcBallController> scene_arcball;

// Camera and light
component::PerspectiveCameraPtr camComp;
component::LightComponentPtr point_light_comp;
light::PointLightPtr point_light;

shader::ShaderPtr phong_shader;

// Framebuffers
framebuffer::FramebufferPtr shadowFBO;
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

// Helper macros for SceneBuilder syntax (matches reference style)
#define sgA scene::graph()->buildAt("sgA_root")
#define sgB scene::graph()->buildAt("sgB_root")
#define sgC scene::graph()->buildAt("sgC_root")

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
        std::cout << "=== Project 3: Reflections and Shadows ===" << std::endl;
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
            // Add uniform to control shadow rendering mode
            phong_shader->configureStaticUniform<bool>("u_renderShadow", []() { return false; });
            material::stack()->defineDefault("u_material_alpha", 1.0f); // Default alpha uniform
            material::stack()->configureShaderDefaults(phong_shader);
            phong_shader->Bake();

        } catch (const std::exception& e) {
            std::cerr << "Shader compilation failed: " << e.what() << std::endl;
            throw;
        }

        // 3. Create framebuffers
        // Shadow FBO
        shadowFBO = framebuffer::Framebuffer::MakeShadowMap(
            2048, 2048, 
            "shadow_depth_map", 
            framebuffer::attachment::Format::DepthComponent24
        );

        // Reflection FBO
        reflectionFBO = framebuffer::Framebuffer::MakeRenderToTexture(
            REFLECTION_WIDTH, REFLECTION_HEIGHT,
            "reflectionColor",  // texture name
            framebuffer::attachment::Format::RGB8,
            framebuffer::attachment::Format::DepthComponent24
        );
       
        
        // 4. Create Geometries
        auto cube_geom = Cube::Make();
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);

        // Create separate scene graph roots
        scene::graph()->addNode("sgA_root");
        scene::graph()->addNode("sgB_root");
        scene::graph()->addNode("sgC_root");

        // Build sgA: cube and sphere
        {
            sgA.addNode("sgA_cube")
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                        ->translate(-1.5f, 0.0f, 0.5f)
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
                    transform::Transform::Make()->translate(0.0f, -1.0f, -0.7f)->scale(0.6f,0.6f,0.6f)
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
            camComp = component::PerspectiveCamera::Make(60.0f, 1.0f, 100.0f);
            sgA.addNode("sgA_cam")
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

        // Build sgB: a single plane (shadow receiver)
        {
            sgB.addNode("sgB_plane")
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                        ->translate(0.0f, -2.0f, 0.0f)
                        ->scale(10.0f,0.1f,10.0f)
                )
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.4f,0.8f,0.4f))
                        ->setDiffuse(glm::vec4(0.4f, 0.8f, 0.4f, 0.5f)) // Semi-transparent
                        ->set<float>("u_material_alpha", 0.5f)
                )
                .with<component::GeometryComponent>(cube_geom);
        }

        // Build sgC: reflection plane (perpendicular to sgB)
        {
            auto sgC_transform = transform::Transform::Make();
            sgC_transform->translate(0.0f, 3.0f, -4.0f);
            sgC_transform->rotate(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate 90 degrees around X-axis to make it vertical
            sgC_transform->scale(10.0f, 0.1f, 8.0f);
            
            sgC.addNode("sgC_plane")
                .with<component::TransformComponent>(sgC_transform)
                .with<component::ShaderComponent>(phong_shader)
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.4f,0.8f,0.4f))
                        ->setDiffuse(glm::vec4(0.4f, 0.8f, 0.4f, 0.5f)) // Semi-transparent
                        ->set<float>("u_material_alpha", 0.5f)
                )
                .with<component::GeometryComponent>(cube_geom);
        }

        // 6. Common Components
        auto phong_shader_comp = component::ShaderComponent::Make(phong_shader);
        auto no_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");

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

        std::cout << "✓ Scenes initialized." << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  '1' - Table Scene" << std::endl;
        std::cout << "  '2' - Solar System Scene" << std::endl;
        std::cout << "  'C' - Switch cameras (Solar System only)" << std::endl;
    };

    auto on_update = [&](double dt) {
        // update logic
    };

    // ==================================================================================
    // PRE-CONFIGURATION OF RENDER STATES
    // ==================================================================================
    
    // State 1: Stencil Masking (Write to stencil buffer)
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

    // State 3: Blending (For semi-transparent surfaces)
    auto blendState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        s->blend().setEnabled(true);
        // Equivalent to: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        s->blend().setFunction(framebuffer::BlendFactor::SrcAlpha, 
                            framebuffer::BlendFactor::OneMinusSrcAlpha);
        return s;
    }();

    // Shadow-specific states
    auto markShadowState = [](){
        auto s = std::make_shared<framebuffer::RenderState>();
        s->stencil().setTest(true);
        s->stencil().setFunction(framebuffer::StencilFunc::Never, 1, 0xFFFF);
        s->stencil().setOperation(framebuffer::StencilOp::Replace, 
                                  framebuffer::StencilOp::Replace, 
                                  framebuffer::StencilOp::Replace);
        return s;
    }();

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

    // Render: Combined reflection and shadow rendering
    auto on_render = [&](double alpha) {
        // 0. Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // ==================================================================================
        // REFLECTION PASSES
        // ==================================================================================

        // Step 1: Mark reflector plane (sgC) in stencil
        framebuffer::stack()->push(nullptr, maskState); 
        {
            scene::graph()->drawSubtree("sgC_root");
        }
        framebuffer::stack()->pop();

        // Step 2: Draw reflected scene where stencil == 1
        framebuffer::stack()->push(nullptr, reflectionState);
        {
            // Calculate reflection matrix for the rotated plane
            // Plane is at (0, 1, 8) and rotated 90° around X-axis
            // After 90° rotation around X: normal = (0, cos(90°), sin(90°)) = (0, 0, 1)
            // This creates a vertical plane perpendicular to Z-axis at z = 8
            
            glm::mat4 reflectionMatrix = glm::mat4(1.0f);
            reflectionMatrix[2][2] = -1.0f;  // Scale Z by -1 (mirror across Z)
            reflectionMatrix[3][2] = -8.0f;  // Translate Z by 16 (2 * planeZ where planeZ = 8)
            
            transform::stack()->push(reflectionMatrix);
            
            // With negative scale on Z, winding order is reversed
            glFrontFace(GL_CW);
            
            // Draw the reflected scene WITHOUT shadows
            // Only draw objects and receiver plane
            
            // Draw scene without shadow receivers (sgA)
            scene::graph()->drawSubtree("sgA_root");
            
            // Draw shadow receiver (sgB)
            scene::graph()->drawSubtree("sgB_root");
            
            glFrontFace(GL_CCW);
            transform::stack()->pop();
        }
        framebuffer::stack()->pop();

        // ==================================================================================
        // NORMAL SCENE WITH SHADOWS
        // ==================================================================================

        // Step 3: Draw scene without shadow receivers (sgA)
        scene::graph()->drawSubtree("sgA_root");
        
        // Step 4: Draw shadow receiver with ambient light only (sgB)
        scene::graph()->drawSubtree("sgB_root");
        
        // Step 5: Mark shadows in stencil and illuminate
        glm::vec4 lightPos = point_light_comp->getWorldTransform() * point_light->getPosition();
        glm::vec4 planeNormal = glm::vec4(0.0f, 1.0f, 0.0f, 2.0f);
        
        // Step 5a: Mark shadows in stencil buffer
        framebuffer::stack()->push(nullptr, markShadowState);
        {
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
            transform::stack()->push(shadowProj);
            scene::graph()->drawSubtree("sgA_root");
            transform::stack()->pop();
        }
        framebuffer::stack()->pop();
        
        // Step 5b: Illuminate receiver where stencil is NOT marked (no shadow)
        framebuffer::stack()->push(nullptr, illuminateState);
        {
            scene::graph()->drawSubtree("sgB_root");
        }
        framebuffer::stack()->pop();

        // ==================================================================================
        // DRAW REFLECTOR PLANE
        // ==================================================================================

        // Step 6: Draw reflector plane (sgC) with blending
        framebuffer::stack()->push(nullptr, blendState);
        {
            scene::graph()->drawSubtree("sgC_root");
        }
        framebuffer::stack()->pop();

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