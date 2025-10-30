#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <gl_base/material.h>
#include <components/all.h>
#include <components/light_component.h>
#include <components/material_component.h>
#include <3d/lights/light_manager.h>
#include <3d/lights/point_light.h>
#include <3d/camera/perspective_camera.h>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f


int main() {

    auto* handler = new input::InputHandler();
    std::shared_ptr<arcball::ArcBallInputHandler> arcball_handler;

    auto on_init = [&](engene::EnGene& app) {
        // Configure the uniforms from the base shader
        app.getBaseShader()->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        light::manager().bindToShader(app.getBaseShader());
        
        // Set material uniform names to match our shader
        material::Material::SetDefaultAmbientName("u_material_ambient");
        material::Material::SetDefaultDiffuseName("u_material_diffuse");
        material::Material::SetDefaultSpecularName("u_material_specular");
        material::Material::SetDefaultShininessName("u_material_shininess");
        
        // Configure material system with the shader
        material::stack()->configureShaderDefaults(app.getBaseShader());

        // Base plane (ground) - white/light gray with matte appearance
        scene::graph()->addNode("base plane")
            .with<component::GeometryComponent>(
                Cube::Make(),
                "base_plane"
            )
            .with<component::MaterialComponent>(
                material::Material::Make(glm::vec3(0.9f, 0.9f, 0.9f))
                    ->setShininess(16.0f)
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()
                ->translate(0, -0.6, 0)
                ->scale(5.0, 0.1, 5.0)
            );

        // Central cube (yellow/gold)
        scene::graph()->addNode("cube translate")
            .with<component::ObservedTransformComponent>(
                transform::Transform::Make()
                ->translate(0, -0.5, -0.4), // CORRECT
                "cube transform"
            )
            .addNode("cube")
                .with<component::GeometryComponent>(
                    Cube::Make(),
                    "cube"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(1.0f, 0.84f, 0.0f))
                        ->setShininess(32.0f)
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->rotate(25, 0, 1, 0)
                    ->scale(0.8, 0.8, 0.8)
                );
        
        // Top sphere (light green) - on top of cube with high shininess
        scene::graph()->buildAt("cube translate")
            .addNode("top sphere")
                .with<component::GeometryComponent>(
                    Sphere::Make(32, 32),
                    "top_sphere"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.5f, 1.0f, 0.5f))
                        ->setShininess(64.0f)
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->translate(0, 1.15, 0) // CHANGED: Was 0.975
                    ->scale(0.35, 0.35, 0.35)
                );
        
        // Side sphere (red/pink) - beside the cube with high shininess
        scene::graph()->addNode("side sphere")
            .with<component::GeometryComponent>(
                Sphere::Make(32, 32),
                "side_sphere"
            )
            .with<component::MaterialComponent>(
                material::Material::Make(glm::vec3(1.0f, 0.4f, 0.5f))
                    ->setShininess(64.0f)
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()
                ->translate(1.2, 0.05, 0) // CHANGED: Was -0.225
                ->scale(0.55, 0.55, 0.55)
            );
        
        // Point light
        light::PointLightParams p;
        p.ambient = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        p.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        p.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        scene::graph()->addNode("light")
            .with<component::LightComponent>(
                light::PointLight::Make(p),
                transform::Transform::Make()
                ->translate(2.0, 2.0, 1.5)
            )
            .addNode("light visualizer")
                .with<component::GeometryComponent>(
                    Sphere::Make(8, 8),
                    "light_visualizer"
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->scale(0.1, 0.1, 0.1)
                );

        light::manager().apply();

        // Setup camera
        scene::graph()->addNode("camera node")
            .with<component::PerspectiveCamera>();

        scene::graph()->setActiveCamera("camera node");
        scene::graph()->getActiveCamera()->setAspectRatio(800.0f / 800.0f);
        scene::graph()->getActiveCamera()->setTarget(
            scene::graph()->getNodeByName("cube translate")->payload().get<component::ObservedTransformComponent>("cube transform")
        );

        // Attach arcball controls
        arcball_handler = arcball::attachArcballTo(*handler);
    };

    // This function handles the fixed-timestep simulation logic.
    auto on_fixed_update = [&](double fixed_timestep) {

    };

    // This function handles all rendering.
    auto on_render = [&](double alpha) {
        // The 'alpha' parameter can be used for smooth interpolation between physics states
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        arcball_handler->syncWithCameraTarget();
        
        // Render the scene graph with the updated positions.
        scene::graph()->draw();
        
        GL_CHECK("render");
    };

    try {
        engene::EnGeneConfig config;
        config.width = 800;
        config.height = 800;
        config.title = "Lighting Engine Demo";
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        config.base_vertex_shader_source = "shaders/lit_vertex.glsl";
        config.base_fragment_shader_source = "shaders/lit_fragment.glsl";

        bool m_wireframe_mode = false;

        handler->registerCallback<input::InputType::KEY>([&](KEY_HANDLER_ARGS){
                
            if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            // Toggle wireframe mode
            else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                m_wireframe_mode = !m_wireframe_mode;
                if (m_wireframe_mode) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                std::cout << "Wireframe mode " << (m_wireframe_mode ? "ON" : "OFF") << std::endl;
            }
            else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
                scene::graph()->clearGraph();
            }
        });

        engene::EnGene app(
            on_init,
            on_fixed_update,
            on_render,
            config,
            handler
        );
        
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}