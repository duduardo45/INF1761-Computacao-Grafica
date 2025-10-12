#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
// <<< No longer need the plain circle, just the textured one
#include <other_genes/textured_shapes/textured_circle.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <components/all.h>

// <<< Include your physics engine files
#include "physics/engine.h"
#include "physics/physicsBody.h"

#include <random> // For generating random positions

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f

// <<< Declare the physics engine pointer so it can be accessed by on_init and on_update
EnginePtr physicsEngine;
int circle_count = 0;

// Function to create a circle with a physics body
void createPhysicsCircle(const glm::vec2& initialPosition, float radius, shader::ShaderPtr shader)
{
    // 1. Create the transform that will be shared between the scene node and the physics body.
    auto circle_transform = transform::Transform::Make();
    
    // 2. Create the visual representation (the scene node).
    scene::graph()->addNode("Circle" + std::to_string(circle_count))
        .with<component::GeometryComponent>(
            TexturedCircle::Make(
                0.0f, 0.0f,  // Center pos (local to the node)
                radius,      // Radius
                32,          // Segments
                0.5f, 0.5f,  // Texture scale/offset if needed
                0.45f
            )
        )
        .with<component::ShaderComponent>(shader)
        .with<component::TextureComponent>(
            texture::Texture::Make("../assets/images/earth_from_space.jpg"), // Using earth texture for all
            "tex",
            0
        )
        // This component holds the transform that the physics engine will update.
        .with<component::TransformComponent>(circle_transform);

    circle_count++;

    // 3. Create the physics body.
    // Pass the initial position, the shared transform, and the radius.
    auto physics_body = PhysicsBody::Make(initialPosition, circle_transform, radius);

    // 4. Add the new body to the engine.
    physicsEngine->addBody(physics_body);
}


int main() {

    // Setup for random positions
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(-0.8f, 0.8f);
    std::uniform_real_distribution<> radii(0.0f, 0.3f);

    shader::ShaderPtr textured_shader;

    auto on_init = [&](engene::EnGene& app) {
        // configures the uniforms from the base shader.
        app.getBaseShader()->configureUniform<glm::mat4>("M", transform::current);

        // creates the texture shader and configures its uniforms
        textured_shader = shader::Shader::Make(
            "../shaders/textured_vertex.glsl",
            "../shaders/textured_fragment.glsl"
        )
        ->configureUniform<glm::mat4>("M", transform::current)
        ->configureUniform<int>("tex", texture::getUnitProvider("tex"));

        // <<< 1. Initialize the Physics Engine
        // We define the simulation area to match the typical OpenGL normalized device coordinates.
        physicsEngine = Engine::make(-1.0f, 1.0f, -1.0f, 1.0f, glm::vec2(0.0f, -1.0f));

        // <<< 2. Create multiple circles with physics bodies

        int numberOfCircles = 10;
        for(int i = 0; i < numberOfCircles; ++i)
        {
            glm::vec2 pos(distr(gen), distr(gen)); // Random initial position
            createPhysicsCircle(pos, radii(gen), textured_shader);
        }
    };

    auto on_update = [&](double time_elapsed) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // <<< 3. Update the physics engine every frame
        // This will calculate new positions based on gravity and collisions.
        // Because we linked the transforms, the visual objects will move automatically.
        if(physicsEngine)
        {
            physicsEngine->update(static_cast<float>(time_elapsed));
        }

        // <<< The old manual rotation code is no longer needed.
        // if (sun_rotation) { ... }
        // if (earth_orbit) { ... }
        
        // Render the scene graph with the updated positions.
        scene::graph()->draw();
        
        Error::Check("update");
    };

    try {
        engene::EnGeneConfig config;
        config.width = 800;
        config.height = 800;
        config.title = "Physics Engine Demo";
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        config.base_vertex_shader_path = "../shaders/vertex.glsl";
        config.base_fragment_shader_path = "../shaders/fragment.glsl";

        auto* handler = new input::BasicInputHandler();
        handler->registerCallback<input::InputType::MOUSE_BUTTON>([&](MOUSE_BUTTON_HANDLER_ARGS) {
            if (action == GLFW_PRESS) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);

                int fb_w, fb_h;
                glfwGetFramebufferSize(window, &fb_w, &fb_h);
                
                // Convert screen coordinates (pixels) to Normalized Device Coordinates (NDC) [-1, 1]
                float x_ndc = ((float)xpos / (float)fb_w) * 2.0f - 1.0f;
                float y_ndc = (1.0f - ((float)ypos / (float)fb_h)) * 2.0f - 1.0f;

                glm::vec2 pos(x_ndc,y_ndc);
                createPhysicsCircle(pos, radii(gen), textured_shader);
            }
        });

        handler->registerCallback<input::InputType::CURSOR_POSITION>([](CURSOR_POS_HANDLER_ARGS){});

        engene::EnGene app(
            config,
            handler,
            on_init,
            on_update
        );
        
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}