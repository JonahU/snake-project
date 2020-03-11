#include <deque>
#include <chrono>
#include <ncurses.h>
#include <thread>
#include <memory>
#include <stdexcept>

#include <iostream> // TODO: remove later

using namespace std::literals::chrono_literals;
using std::shared_ptr;
using std::make_shared;

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
            next_pos.y --;
            break;
        case Direction::Down:
            next_pos.y ++;
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

    Coordinates update() {
        my_snake.move();
        return my_snake.get_head();
    }

    int who() {
        return identifier;
    }
};

class GameWindow
{
    Coordinates player1_start;
    Coordinates player2_start;
    std::thread input_thread;
    shared_ptr<Player> player_1;
    shared_ptr<Player> player_2;

    void input_handler() {
        /*
        Originally each player had its own input_handler & input_thread. 
        However, ncurses is not thread safe, and calling getch() from 
        multiple threads led to weird results. Therefore, I moved input 
        handling into the GameWindow class.
        */
        while(1) {
            const int ch = getch();
            player_1->handle_key_press(ch);
            player_2->handle_key_press(ch);
        }
    }
public:
    GameWindow() : player_1(nullptr), player_2(nullptr) {
        initscr(); // start curses mode
        cbreak(); // disable line buffering
        keypad(stdscr, TRUE); // allow arrow & fn keys
        noecho(); // turn off echoing
        curs_set(0); // hide the cursor 

        int max_x;
        int max_y;
        getmaxyx(stdscr, max_y, max_x); // get terminal dimensions
        int half_y = max_y/2;
        int quarter_x = max_x / 4;
        int three_quarter_x = quarter_x*3;
        player1_start = {quarter_x, half_y};
        player2_start = {three_quarter_x, half_y};
    };
    GameWindow(const GameWindow&) = delete;
    ~GameWindow() {
        if (input_thread.joinable())
            input_thread.join();
        endwin();
    }

    void set_players(shared_ptr<Player> p1, shared_ptr<Player> p2) {
        player_1 = p1;
        player_2 = p2;
    }

    void start() {
        if (player_1 == nullptr || player_2 == nullptr) {
            throw std::runtime_error("game_window::start called before setting players");
        }
        input_thread = std::thread(&GameWindow::input_handler, this);
    }

    Coordinates get_player1_start() {
        return player1_start;
    }

    Coordinates get_player2_start() {
        return player2_start;
    }

    void update(Coordinates p1_pos) {
        wclear(stdscr);
        mvwaddstr(stdscr, p1_pos.y, p1_pos.x, "1");
    }

    void update(Coordinates p1_pos, Coordinates p2_pos) {
        wclear(stdscr);
        mvwaddstr(stdscr, p1_pos.y, p1_pos.x, "1");
        mvwaddstr(stdscr, p2_pos.y, p2_pos.x, "2");
    }

    void render() {
        wrefresh(stdscr);
    }
};

class Game {
    GameWindow game_window;
    shared_ptr<Player> player_1;
    shared_ptr<Player> player_2;
    bool game_over;
public:
    Game() :
        player_1(make_shared<Player>(1, game_window.get_player1_start(), Direction::Right, 'w', 's', 'a', 'd')),
        player_2(make_shared<Player>(2, game_window.get_player2_start(), Direction::Left, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT)),
        game_over(false)
    {
        game_window.set_players(player_1, player_2);
    }

    void start() {
        game_window.start(); // spin up input thread
        while (!game_over)
        {
            auto start_time = std::chrono::steady_clock::now();

            // 1) UPDATE (producer?)
            update();
            // 2) RENDER (consumer?)
            game_window.render();

            auto finish_time = std::chrono::steady_clock::now();
            auto time_taken = finish_time-start_time;
            auto time_taken_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_taken);
            std::this_thread::sleep_for(50ms - time_taken_milliseconds); // 20 fps
        }
        
    }

    void update() {
        Coordinates p1_pos = player_1->update();
        Coordinates p2_pos = player_2->update();
        game_window.update(p1_pos, p2_pos);
        // game_window.update(p1_pos);
        // TODO: check for winner
    }
};

int main() {
    Game game;
    game.start();
}