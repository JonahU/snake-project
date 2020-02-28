#include <deque>
#include <chrono>
#include <ncurses.h>
#include <thread>

#include <iostream> // TODO: remove later

using namespace std::literals::chrono_literals;

enum class Direction {
    None, Up, Down, Left, Right
};

struct Coordinates { int x, y; };

class Snake {
    Direction direction;
    int length;
    std::deque<Coordinates> position;

    Coordinates get_next_pos() {
        auto next_pos = get_head();
        switch (direction)
        {
        case Direction::Up:
            next_pos.y ++;
            break;
        case Direction::Down:
            next_pos.y --;
            break;
        case Direction::Left:
            next_pos.x --;
            break;
        case Direction::Right:
            next_pos.x ++;
            break;
        default:
            break;
        }
        return next_pos;
    }
public:
    Snake(Coordinates start_pos, Direction start_dir, int len) :length(len), direction(start_dir) {
        position.push_front(start_pos);
    }

    Coordinates get_head() {
        return position.front();
    }

    void change_direction(Direction new_dir) {
        direction = new_dir;
    }

    void move() {
        if(position.size() == length) {
            position.pop_back();
        }
        position.push_front(get_next_pos());
    }
};

class Player {
    int identifier; // Player 1, Player 2 etc.
    Snake my_snake;
    int key_up, key_down, key_left, key_right;
    std::thread input_thread;

    void input_handler() {
        while(1) {
            handle_key_press(getch());
        }
    }

public:
    Player(
        int num,
        Coordinates snake_start_pos,
        Direction snake_start_dir,
        int up,
        int down,
        int left,
        int right,
        int snake_len = 10
        ) :
        identifier(num),
        my_snake(snake_start_pos, snake_start_dir, snake_len),
        key_up(up),
        key_down(down),
        key_left(left),
        key_right(right) 
    {}

    ~Player() {
        if (input_thread.joinable())
            input_thread.join();
    }

    void start() {
        input_thread = std::thread(&Player::input_handler, this);
    }
    
    void handle_key_press(const int input_ch) { 
        if(input_ch == key_up) {
            my_snake.change_direction(Direction::Up);
        } else if (input_ch == key_down) {
            my_snake.change_direction(Direction::Down);
        } else if (input_ch == key_left) {
            my_snake.change_direction(Direction::Left);
        } else if (input_ch == key_right) {
            my_snake.change_direction(Direction::Right);
        }
    }

    void update() {
        my_snake.move();
    }

    int who() {
        return identifier;
    }
};

// TODO: class GameBoard

struct Game {
    Player player_1;
    Player player_2;
    bool game_over;
    Game() :
        player_1(1, {25,50}, Direction::Right, 'w', 's', 'a', 'd'),
        player_2(2, {75,50}, Direction::Left, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT),
        game_over(false)
    {}

    void start() {
        player_1.start(); // spin up player 1 input thread
        player_2.start(); // spin up player 2 input thread
        // GameBoard.start() spin up game render thread

        while (!game_over)
        {
            auto start_time = std::chrono::steady_clock::now();

            // 1) UPDATE
            update();
            // 2) RENDER
            // GameBoard.render()

            auto finish_time = std::chrono::steady_clock::now();
            auto time_taken = finish_time-start_time;
            auto time_taken_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_taken);
            std::this_thread::sleep_for(50ms - time_taken_milliseconds); // 20 fps
        }
        
    }

    void update() {
        player_1.update();
        player_2.update();
        // TODO: check for winner
    }
};

int main() {
    // initscr(); // start curses mode
    // cbreak(); // disable line buffering
    // keypad(stdscr, TRUE); // allow arrow & fn keys
    // noecho(); // turn off echoing
    // curs_set(0); // hide the cursor
    
    Game game;
    game.start();
}