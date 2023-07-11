#ifndef hCamera
#define hCamera

struct camMatrix {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 position;
    camMatrix() {
        position = glm::vec3(2.0f, 1.0f, 2.0f);
        Orientation = glm::vec3(-2.f, 0.f, -2.f);
        Up = glm::vec3(0.f, 1.f, 0.f);

        view = glm::mat4(1.0f);
        proj = glm::mat4(1.0f);
    }
    void update(GLFWwindow* window, VkExtent2D extent, float FOVdeg, float nearPlane, float farPlane)
    {	// Initializes matrices since otherwise they will be the null matrix
        if (!glfwJoystickPresent(GLFW_JOYSTICK_1)) {
            keyboard_Input(window, extent);
        }
        else {
            keyboard_Input(window, extent);
            controller_Input(window);
        }
        view = glm::lookAt(position, position + Orientation, Up);
        proj = glm::perspective(glm::radians(FOVdeg), (float)extent.width / extent.height, nearPlane, farPlane);
        proj[1][1] *= -1;
    }
protected:
    glm::vec3 Orientation;
    glm::vec3 Up;
    float velocity = 0.05f;
    float sensitivity = 1.75f;
private:
    bool firstClick = true;
    void keyboard_Input(GLFWwindow* window, VkExtent2D extent) {
        if (glfwGetKey(window, GLFW_KEY_W))
        {
            position += velocity * Orientation;
        }

        if (glfwGetKey(window, GLFW_KEY_A))
        {
            position -= velocity * glm::normalize(glm::cross(Orientation, Up));
        }

        if (glfwGetKey(window, GLFW_KEY_S))
        {
            position -= velocity * Orientation;
        }

        if (glfwGetKey(window, GLFW_KEY_D))
        {
            position += velocity * glm::normalize(glm::cross(Orientation, Up));
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE))
        {
            position += velocity * Up;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
        {
            position -= velocity * Up;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
        {
            velocity = 0.01f;
        }

        else if (!glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
        {
            velocity = 0.005f;
        }

        // Handles mouse inputs
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
        {	// Hides mouse cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            if (firstClick)
            {	// Prevents camera from jumping on the first click
                glfwSetCursorPos(window, (extent.width / 2), (extent.height / 2));
                firstClick = false;
            }
            // Stores the coordinates of the cursor
            double mouseX;
            double mouseY;

            // Fetches the coordinates of the cursor
            glfwGetCursorPos(window, &mouseX, &mouseY);

            // Normalizes and shifts the coordinates of the cursor such that they begin in the middle of the screen
            // and then "transforms" them into degrees 
            float rotX = sensitivity * (float)(mouseY - (extent.height / 2)) / extent.height;
            float rotY = sensitivity * (float)(mouseX - (extent.width / 2)) / extent.width;

            // Calculates upcoming vertical change in the Orientation
            glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, Up)));

            if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
            {	// Decides whether or not the next vertical Orientation is legal or not
                Orientation = newOrientation;
            }

            // Rotates the Orientation left and right
            Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);

            // Sets mouse cursor to the middle of the screen so that it doesn't end up roaming around
            glfwSetCursorPos(window, (extent.width / 2), (extent.height / 2));
        }
        else if (!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
        {	// Unhides cursor since camera is not looking around anymore
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // Makes sure the next time the camera looks around it doesn't jump
            firstClick = true;
        }
    }
    void controller_Input(GLFWwindow* window)
    {
        // Button Mapping
        int buttonCount;
        const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonCount);
        if (buttons[1])
        {// X button
            position += 2 * velocity * Up;
        }
        if (buttons[2])
        {
            position -= 2 * velocity * Up;
        }
        if (buttons[10])
        {// left stick in
            velocity *= 10.f;
        }
        if (buttons[11])
        {// right stick in
            velocity *= 10.f;
        }
        if (buttons[14])
        {// dpad up
            velocity *= 100.0f;
        }
        if (buttons[15])// dpad down
        {
            velocity /= 2.f;
        }
        if (buttons[5])// right bumper
        {
            float rotZ = sensitivity;
            glm::mat4 roll_mat = glm::rotate(glm::mat4(1.0f), glm::radians(rotZ), Orientation);
            Up = glm::mat3(roll_mat) * Up;
        }
        if (buttons[4])
        {
            float rotZ = -sensitivity;
            glm::mat4 roll_mat = glm::rotate(glm::mat4(1.0f), glm::radians(rotZ), Orientation);
            Up = glm::mat3(roll_mat) * Up;
        }

        // Controller Joystick Settings
        int axesCount;
        const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);
        // Left and Right
        if ((axes[0] > 0.1) || (axes[0] < -0.1))
        {
            position += velocity * axes[0] * glm::normalize(glm::cross(Orientation, Up));
        }
        // Forward and Backwards
        if ((axes[1] > 0.1) || (axes[1] < -0.1))
        {
            position -= velocity * axes[1] * Orientation;
        }
        if ((axes[2] > 0.1) || (axes[2] < -0.1))
        {
            float rotX = sensitivity * axes[2];
            Orientation = glm::rotate(Orientation, glm::radians(-rotX), Up);

        }
        if ((axes[5] > 0.15) || (axes[5] < -0.15))
        {
            float rotY = sensitivity * axes[5];
            // Calculates upcoming vertical change in the Orientation
            glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotY), glm::normalize(glm::cross(Orientation, Up)));

            if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
            {	// Decides whether or not the next vertical Orientation is legal or not
                Orientation = newOrientation;
            }
        }
    }
};

#endif