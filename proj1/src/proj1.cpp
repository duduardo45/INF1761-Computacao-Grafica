#include <EnGene.h>
#include <scene.h>
#include <scene_visitor.h>
#include <shapes/circle.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <components/all.h>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f

int main() {
    // Define the user's one-time initialization logic
    auto on_init = []() {
        // This is the code that runs once at the start.
        scene::graph()->initializeBaseShader("../shaders/vertex.glsl", "../shaders/fragment.glsl");
        scene::graph()->getBaseShader()->configureUniform<glm::mat4>("M", transform::current);
        // ... set up your initial scene, load models, etc.
        scene::SceneGraphVisitor v;

        v.addNode("Sun");
        v.addComponentToCurrentNode(
            component::GeometryComponent::Make(
                Circle::Make(
                    0.0f, 0.0f, // pos centro
                    0.3f, // raio
                    (float[]) { // cores
                        0.8f, 0.5f, 0.0f, // centro
                        BACKGROUND_COLOR, // borda
                    },
                    32,
                    true
                )
            )
        );
        transform::TransformPtr rotacao_sol = transform::Transform::Make();
        rotacao_sol->rotate(30, 0,0,1);
        v.addComponentToCurrentNode(
            component::TransformComponent::Make(
                rotacao_sol
            )
        );

        v.goToRoot();

        v.addNode("sun earth distance");
        transform::TransformPtr dist_sol_terra = transform::Transform::Make();
        dist_sol_terra->translate(0.7f, 0.0f, 0.0f);
        v.addComponentToCurrentNode(
            component::TransformComponent::Make(
                dist_sol_terra
            )
        );

        v.addNode("Earth");
        v.addComponentToCurrentNode(
            component::GeometryComponent::Make(
                Circle::Make(
                    0.0f, 0.0f, // pos centro
                    0.1f, // raio
                    (float[]) { // cores
                        0.0f, 0.1f, 0.5f // centro
                    },
                    32,
                    false
                )
            )
        );
    };

    // Define the user's per-frame update and drawing logic
    auto on_update = []() {
        // This code runs every frame.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // ... update object positions, handle animations ...
        
        scene::graph()->draw();
        
        Error::Check("update");
    };

    try {
        // 1. Create your input handler instance.
        auto* handler = new input::BasicInputHandler();

        // 2. Create the EnGene instance, passing in all your configurations.
        engene::EnGene app(
            800, 800,                           // Window dimensions
            "My Awesome EnGene App",            // Window title
            handler,                            // The input handler
            on_init,                            // Your init function
            on_update,                          // Your update function
            60,                                 // Max framerate
            (float[]) {BACKGROUND_COLOR, 1.0f}
        );

        
        // 3. Run the application. This call blocks until the window is closed.
        app.run();
        

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}