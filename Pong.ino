#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Convert enum class instances to int
// Pre-condition: assumes that int is the underlying type of the enum class
template<typename T>
constexpr int toInt(T info) noexcept
{
    return static_cast<int>(info);
}

// enum class Definitions
enum class GameState
{
    Running, GameOver
};

enum class GameInvariants
{
    Width = 128, Height = 32, MaxScore = 4
};

// Game Classes Definitions: Paddle, Score, GameObject, Ball and Renderer
class Paddle
{
public:
    explicit Paddle(int pos): position(pos)
    {}

    Paddle(): Paddle(0)
    {}

    int getPosition() const
    {
        return position;
    }

    void setPosition(int pos)
    {
        if (pos < 0 || pos > toInt(GameInvariants::Height))
        {
            return;
        }

        position = pos;
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }
private:
    int position;
    const int width = 2;
    const int height = 9;
};

class Score
{
public:
    explicit Score(int initialScore): score(initialScore)
    {}

    Score(): Score(0)
    {}

    int get() const
    {
        return score;
    }

    void increase()
    {
        if (score + 1 > toInt(GameInvariants::MaxScore))
        {
            return;
        }

        ++score;
    }
private:
    int score;
};

/**
* struct GameObject implementation using Composition relationship ("has-a").
* In this game, a GameObject only has a Paddle and a Score, so we keep it simple.
**/
struct GameObject
{
    Paddle paddle;
    Score score;
};

class Ball
{
public:
    Ball(int posX, int posY): x(posX), y(posY)
    {}

    Ball(): Ball(0, 0)
    {}

    void reset(int posX, int posY)
    {
        if (x < 0 || x > toInt(GameInvariants::Width) || y < 0 || y > toInt(GameInvariants::Height))
        {
            return;
        }

        x = posX;
        y = posY;
    }

    void update()
    {
        x += horizontalSpeed;
        y += verticalSpeed;
    }

    int getX() const
    {
        return x;
    }

    int getY() const
    {
        return y;
    }

    void flipVerticalDir()
    {
        verticalSpeed *= -1;
    }

    void flipHorizontalDir()
    {
        horizontalSpeed *= -1;
    }

    int getHorizontalDir() const
    {
        return horizontalSpeed;
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }
private:
    int x, y;
    int horizontalSpeed = 4;
    int verticalSpeed = 2;
    const int width = 2;
    const int height = 2;
};

class Renderer
{
public:
    Renderer(): renderTarget(4)
    {
        setTextProperties(1, WHITE);
    }

    void start()
    {
        renderTarget.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initializes the target display
    }

    void render()
    {
        renderTarget.display();
    }

    void makeRect(int x, int y, int w, int h, int color)
    {
        renderTarget.fillRect(x, y, w, h, color);
    }

    void clear()
    {
        renderTarget.clearDisplay();
    }

    void setCursor(int x, int y)
    {
        renderTarget.setCursor(x, y);
    }

    template<typename T>
    void printMessage(const T& message)
    {
        renderTarget.print(message);
    }
private:
    Adafruit_SSD1306 renderTarget;

    void setTextProperties(int size, int color)
    {
        renderTarget.setTextSize(size);
        renderTarget.setTextColor(color);
    }
};

// Game Variables Definition
GameState currentState = GameState::Running;
Renderer renderer;
GameObject player, cpu;
Ball ball(toInt(GameInvariants::Width) / 2, toInt(GameInvariants::Height) / 2);

void setup()
{
    Serial.begin(9600);
    renderer.start();
    renderer.clear();
}

void loop()
{
    if (currentState == GameState::Running)
    {
        cpu.paddle.setPosition(aiPosition());

        player.paddle.setPosition(positionFromPotentiometer());

        collisionResolution(); // Handles collision between objects
        ball.update();
    }
    else if (currentState == GameState::GameOver)
    {
        renderer.setCursor(toInt(GameInvariants::Width) / 16, toInt(GameInvariants::Height) / 2);

        if (player.score.get() == toInt(GameInvariants::MaxScore))
        {
            renderer.printMessage("Winner!");
        }
        else if (cpu.score.get() == toInt(GameInvariants::MaxScore))
        {
            renderer.printMessage("Loser!");
        }
    }

    drawPaddles();
    drawScore();
    drawCentralLine();
    drawBall();
    renderer.render();
    renderer.clear();
}

int positionFromPotentiometer()
{
    // The data read from the potentiometer will be an int in the range [0; 1023]
    int potentiometerData = analogRead(A0);

    /**
    * The highest possible position to the paddle is GameInvariants::Height - paddle.getHeight(),
    * which, in this case, is 32 - 9 = 23.
    * So the position will be in the range [0, 23], and it varies accordingly to the value stored in
    * potentiometerData.
    **/

    return (potentiometerData * (23.0 / 1023.0));
}

// Return the new position for the CPU's controlled paddle
int aiPosition()
{
    /**
    * The new CPU-controlled paddle position will be equal to the ball vertical position (ball.getY())
    * with an offset; without such offset, the CPU would always win.
    *
    * We pick two (pseudo)random numbers: one for the direction (up or down) and another for
    * the displacement in the chosen direction (between 0 and 3).
    * The result is that we randomly choose a direction and slightly move the CPU-controlled paddle in that
    * direction, as if the paddle was indeed trying to follow the ball.
    **/

    int randDir = (((rand() % 41) % 2) ? 1 : -1);
    int randDisplacement = rand() % 4;
    return (ball.getY() + randDir * randDisplacement);
}

void drawPaddles()
{
    // Player's Paddle Rectangle
    renderer.makeRect(0, player.paddle.getPosition(), player.paddle.getWidth(), player.paddle.getHeight(), WHITE);

    // CPU-controlled Paddle Rectangle
    renderer.makeRect(toInt(GameInvariants::Width) - cpu.paddle.getWidth(), cpu.paddle.getPosition(),
                      cpu.paddle.getWidth(), cpu.paddle.getHeight(), WHITE);
}

void drawScore()
{
    renderer.setCursor(55, 0);
    renderer.printMessage(player.score.get());
    renderer.printMessage(":");
    renderer.printMessage(cpu.score.get());
}

void drawCentralLine()
{
    for (int i = 0; i < 60; i += 5)
    {
        // Draw a dashed line in the center of screen
        renderer.makeRect((toInt(GameInvariants::Width) / 2) - 1, 12 + i, 1, 1, WHITE);
    }
}

void collisionResolution()
{
    // Handles vertical collision
    if (ball.getY() >= toInt(GameInvariants::Height) - ball.getHeight() || ball.getY() < 0)
    {
        ball.flipVerticalDir();
    }

    // Handles collision against player's goal
    if (ball.getX() >= toInt(GameInvariants::Width) - ball.getWidth() && ball.getHorizontalDir() > 0)
    {
        player.score.increase();
        ball.reset(3 * (toInt(GameInvariants::Width) / 4), toInt(GameInvariants::Height) / 2);
        ball.flipHorizontalDir();

        if (player.score.get() == toInt(GameInvariants::MaxScore))
        {
            currentState = GameState::GameOver;
        }
    }
    else if (ball.getX() < 0 && ball.getHorizontalDir() < 0) // Handles collision against CPU goal
    {
        cpu.score.increase();
        ball.reset(toInt(GameInvariants::Width) / 4, toInt(GameInvariants::Height) / 2);
        ball.flipHorizontalDir();

        if (cpu.score.get() == toInt(GameInvariants::MaxScore))
        {
            currentState = GameState::GameOver;
        }
    }
    else if (ball.getX() >= player.paddle.getWidth() && ball.getX() <= player.paddle.getWidth() + ball.getWidth() &&
             ball.getHorizontalDir() < 0) // Handles collision against player paddle
    {
        if (ball.getY() > player.paddle.getPosition() - 4 && ball.getY() < player.paddle.getPosition() + player.paddle.getHeight())
        {
            ball.flipHorizontalDir();
        }
    }
    else if (ball.getX() >= toInt(GameInvariants::Width) - player.paddle.getWidth() - ball.getWidth() &&
             ball.getX() <= toInt(GameInvariants::Width) - player.paddle.getWidth() && ball.getHorizontalDir() > 0) // Handles collision against CPU paddle
    {
        if (ball.getY() > cpu.paddle.getPosition() - 2 && ball.getY() < cpu.paddle.getPosition() + cpu.paddle.getHeight())
        {
            ball.flipHorizontalDir();
        }
    }
}

void drawBall()
{
    renderer.makeRect(ball.getX(), ball.getY(), ball.getWidth(), ball.getHeight(), WHITE);
}
