#include <cctype>
#include <chrono>
#include <deque>
#include <memory>
#include <ncurses.h>
#include <stdexcept>
#include <thread>

#include <iostream> // TODO: remove later

using namespace std::literals::chrono_literals;
using std::shared_ptr;
using std::make_shared;

enum class Direction {
    None, Up, Down, Left, Right
};

struct Coordinates { int x, y; };
typedef std::deque<Coordinates> CoordinatesQueue;

class Snake {

    CoordinatesQueue snake_body;
    Direction direction;
    int length;

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

    Direction get_opposite(Direction dir) {
        if (dir == Direction::Up) return Direction::Down;
        else if (dir == Direction::Down) return Direction::Up;
        else if (dir == Direction::Left) return Direction::Right;
        else if (dir == Direction::Right) return Direction::Left;
        else return Direction::None;
    }
public:
    Snake(Coordinates start_pos, Direction start_dir, int len) :length(len), direction(start_dir) {
        snake_body.push_front(start_pos);
    }

    Coordinates const& get_head() {
        return snake_body.front();
    }

    CoordinatesQueue const& get_body() {
        // body includes head
        return snake_body;
    }

    void change_direction(Direction new_dir) {
        if (new_dir != get_opposite(direction))
            direction = new_dir;
    }

    void move() {
        if(snake_body.size() == length) {
            snake_body.pop_back();
        }
        snake_body.push_front(get_next_pos());
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
        if(input_ch == key_up || input_ch == toupper(key_up)) {
            my_snake.change_direction(Direction::Up);
        } else if (input_ch == key_down || input_ch == toupper(key_down)) {
            my_snake.change_direction(Direction::Down);
        } else if (input_ch == key_left || input_ch == toupper(key_left)) {
            my_snake.change_direction(Direction::Left);
        } else if (input_ch == key_right || input_ch == toupper(key_right)) {
            my_snake.change_direction(Direction::Right);
        }
    }

    CoordinatesQueue const& update() {
        my_snake.move();
        return my_snake.get_body();
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
    int P1_COLOR_PAIR = 1;
    int P2_COLOR_PAIR = 2;
    int BACKGROUND_COLOR_PAIR = 3;
    int COLLISION_COLOR_PAIR = 4;

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
        // Initalize curses
        initscr(); // start curses mode
        cbreak(); // disable line buffering
        keypad(stdscr, TRUE); // allow arrow & fn keys
        noecho(); // turn off echoing
        curs_set(0); // hide the cursor

        // Initialize colors
        if (has_colors() == FALSE) {
            throw std::runtime_error("Your terminal does not support color");
        }
        start_color();
        init_pair(P1_COLOR_PAIR, COLOR_WHITE, COLOR_GREEN); // (index, foreground, background)
        init_pair(P2_COLOR_PAIR, COLOR_BLACK, COLOR_BLUE);
        init_pair(BACKGROUND_COLOR_PAIR, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLLISION_COLOR_PAIR, COLOR_RED, COLOR_RED);
        wbkgd(stdscr, COLOR_PAIR(BACKGROUND_COLOR_PAIR)); // set window to background color

        // Calculate player starting positions 
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
        endwin(); // end curses mode
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

    void update(CoordinatesQueue const& p1_pos, CoordinatesQueue const& p2_pos) {
        // could be optimized to only update the head and remove the last element (if necessary)
        wclear(stdscr); // clear the screen
        attron(COLOR_PAIR(P1_COLOR_PAIR));
        for (Coordinates pos: p1_pos) {
            mvwaddch(stdscr, pos.y, pos.x, ' '); // draw new p1 positions
        }
        attroff(COLOR_PAIR(P1_COLOR_PAIR));
        attron(COLOR_PAIR(P2_COLOR_PAIR));
        for (Coordinates pos: p2_pos) {
            mvwaddch(stdscr, pos.y, pos.x, ' '); // draw new p2 positions
        }
        attroff(COLOR_PAIR(P2_COLOR_PAIR));
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
        while (!game_over) // main game loop
        {
            auto start_time = std::chrono::steady_clock::now();
            update(); // update player positions
            render(); // render updated player positions
            auto finish_time = std::chrono::steady_clock::now();
            auto time_taken = finish_time-start_time;
            auto time_taken_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_taken);
            std::this_thread::sleep_for(50ms - time_taken_milliseconds); // game runs at 20 frames per second
        }
        
    }

    void render() {
        game_window.render();
    }

    void update() {
        CoordinatesQueue const& p1_pos = player_1->update();
        CoordinatesQueue const& p2_pos = player_2->update();
        game_window.update(p1_pos, p2_pos);
        // TODO: check for winner
    }
};

int main() {
    Game game;
    game.start();
}