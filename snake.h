#ifndef SNAKE_H
#define SNAKE_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <ncurses.h>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define FRAMES_PER_SECOND 20

using std::find;
using std::shared_ptr;
using std::make_shared;
using std::shared_mutex;
using std::shared_lock;
using std::unique_lock;

namespace snake {

enum class Direction {
    None, Up, Down, Left, Right
};

Direction get_opposite(Direction dir) {
    if (dir == Direction::Up) return Direction::Down;
    else if (dir == Direction::Down) return Direction::Up;
    else if (dir == Direction::Left) return Direction::Right;
    else if (dir == Direction::Right) return Direction::Left;
    else return Direction::None;
}

struct Coordinates {
    int x, y;
    friend bool operator==(Coordinates const& lhs, Coordinates const& rhs);
};
using CoordinatesQueue =  std::deque<Coordinates>;

bool operator==(Coordinates const& lhs, Coordinates const& rhs){
    if ((lhs.x == rhs.x) && (lhs.y == rhs.y)) {
        return true;
    }
    return false;
}

class Snake {
    CoordinatesQueue snake_body;
    Direction direction;
    mutable shared_mutex direction_mutex;
    int length;

    Coordinates get_next_pos() {
        shared_lock slock(direction_mutex); // multiple readers allowed
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
        snake_body.push_front(start_pos);
    }

    Coordinates const& get_head() const {
        return snake_body.front();
    }

    CoordinatesQueue const& get_body() const {
        // body includes head
        return snake_body;
    }

    void change_direction(Direction new_dir) {
        unique_lock ulock(direction_mutex); // only one writer allowed
        if (new_dir != get_opposite(direction))
            direction = new_dir;
    }

    void move() {
        if(snake_body.size() == length) {
            snake_body.pop_back();
        }
        snake_body.push_front(get_next_pos());
    }

    void move(int const& frames_elapsed) {
        if ((frames_elapsed % (2*FRAMES_PER_SECOND)) == 0) {
            // 2 * 20fps, every 2 seconds increase length by 1
            ++length;
        }
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
    
    void handle_key_press(int const input_ch) {
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

    CoordinatesQueue const& update(int const frames_elapsed) {
        my_snake.move(frames_elapsed);
        return my_snake.get_body();
    }

    int who() const {
        return identifier;
    }
};

class GameWindow
{
    Coordinates player1_start, player2_start;
    int initial_height, initial_width;
    std::thread input_thread;
    inline static std::atomic<bool> read_usr_input;
    std::future<int> last_char_typed_f;
    shared_ptr<Player> player_1, player_2;
    std::vector<Coordinates> collision_pos;
    int P1_COLOR_PAIR = 1;
    int P2_COLOR_PAIR = 2;
    int BACKGROUND_COLOR_PAIR = 3;
    int BORDER_COLOR_PAIR = 4;
    int COLLISION_COLOR_PAIR = 5;

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
        init_pair(P1_COLOR_PAIR, COLOR_GREEN, COLOR_GREEN); // (index, foreground, background)
        init_pair(P2_COLOR_PAIR, COLOR_BLUE, COLOR_BLUE);
        init_pair(BACKGROUND_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);
        init_pair(BORDER_COLOR_PAIR, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLLISION_COLOR_PAIR, COLOR_BLACK, COLOR_RED);
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

        // Set initial height & width (width used to calculate starting snake length)
        initial_height = max_y - 3; // (x axis-1) -2 [border height]
        initial_width = max_x - 3; // (y axis-1) - 2 [border width]
    };

    GameWindow(const GameWindow&) = delete;
    GameWindow(GameWindow&&) = delete;
    GameWindow& operator=(const GameWindow&) = delete;
    GameWindow& operator=(GameWindow&&) = delete;

    ~GameWindow() {
        read_usr_input = false; // if true, thread will never join
        if (input_thread.joinable())
            input_thread.join();
        endwin(); // end curses mode
    }

    void input_handler(std::promise<int>&& final_ch_promise) const {
        /*
        Originally each player had its own input_handler & input_thread. 
        However, ncurses is not thread safe, and calling wgetch() from 
        multiple threads led to weird results. Therefore, I moved input 
        handling into the GameWindow class.
        */
        int ch;
        while (read_usr_input) {
            ch = wgetch(stdscr);
            player_1->handle_key_press(ch);
            player_2->handle_key_press(ch);
        }
        final_ch_promise.set_value(ch);
    }

    void draw_border() {
        attron(COLOR_PAIR(BORDER_COLOR_PAIR));
        wborder(
            stdscr, // window to draw border on
            ' ', // left side
            ' ', // right side
            ' ', // top side
            ' ', // bottom side
            ' ', // top left corner
            ' ', // top right corner
            ' ', // bottom left corner
            ' '  // bottom right corner
        );
        attroff(COLOR_PAIR(BORDER_COLOR_PAIR));
    }

    bool did_player_collide(CoordinatesQueue const& player_pos, CoordinatesQueue const& other_player_pos) {
        bool did_collide = false;
        const Coordinates next_pos = player_pos.front();
        const int next_square_color = mvwinch(stdscr, next_pos.y, next_pos.x) & A_COLOR;
        
        // check if collided with a border square
        if (next_square_color == COLOR_PAIR(BORDER_COLOR_PAIR)) {
            did_collide = true;
        }
        // check if collided with self
        if (find(player_pos.begin()+1, player_pos.end(), next_pos) != player_pos.end()) {
            did_collide = true;
        }
        // check if collided with other player
        if (find(other_player_pos.begin(), other_player_pos.end(), next_pos) != other_player_pos.end()) {
            did_collide = true;
        }
        // else player did not collide

        if (did_collide) {
            collision_pos.push_back(next_pos);
        }
        return did_collide;
    }
public:
    static GameWindow& get_instance() {
        static GameWindow single_instance; // GameWindow is a singleton
        return single_instance;
    }

    void set_players(shared_ptr<Player> p1, shared_ptr<Player> p2) {
        player_1 = p1;
        player_2 = p2;
    }

    void start() {
        if (player_1 == nullptr || player_2 == nullptr)
            throw std::runtime_error("game_window::start called before setting players");
        read_usr_input = true;
        if (!input_thread.joinable()) {
            std::promise<int> last_char_input_p;
            last_char_typed_f = last_char_input_p.get_future();
            input_thread = std::thread(&GameWindow::input_handler, this, std::move(last_char_input_p));
        }
    }

    bool play_again() {
        read_usr_input = false;
        int last_char_typed = last_char_typed_f.get();
        input_thread.join();
        switch (last_char_typed) {
            case 'Q': 
            case 'q':
                return false; // quit
            default:
                return true; // restart
        }
    }

    Coordinates get_player1_start() const {
        return player1_start;
    }

    Coordinates get_player2_start() const {
        return player2_start;
    }

    int get_initial_height() const {
        return initial_height;
    }

    int get_initial_width() const {
        return initial_width;
    }

    int update(CoordinatesQueue const& p1_pos, CoordinatesQueue const& p2_pos) {
        // check for collisions
        bool p1_collided = did_player_collide(p1_pos, p2_pos);
        bool p2_collided = did_player_collide(p2_pos, p1_pos);

        wclear(stdscr); // clear the screen
        draw_border(); // draw screen border

        // must redraw every player position every update
        // as wclear copies blanks to every position in the window
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

        // draw collision and return winner if a player collided
        if (p1_collided || p2_collided) {
            for (Coordinates pos : collision_pos) {
                attron(COLOR_PAIR(COLLISION_COLOR_PAIR));
                mvwaddch(stdscr, pos.y, pos.x, ' '); // draw collision square red
                attroff(COLOR_PAIR(COLLISION_COLOR_PAIR));
            }

            if (p1_collided && p2_collided) {
                return 0; // draw
            } else if (p1_collided && !p2_collided) {
                return 2; // p1 lost, winner is p2
            } else if (p2_collided && !p1_collided) {
                return 1; // p2 lost, winner is p1
            }
        }
        return -1; // no winner yet
    }

    void render() {
        wrefresh(stdscr);
    }

    void renderGameOverScreen(int winner) {
        int max_x, max_y;
        getmaxyx(stdscr, max_y, max_x); // get terminal dimensions

        attron(COLOR_PAIR(BORDER_COLOR_PAIR));
        switch (winner) {
        case 2:
             mvwprintw(stdscr, 0, 1, "BLUE WON!");
            break;
        case 1:
             mvwprintw(stdscr, 0, 1, "GREEN WON!");
             break;
        case 0:
            mvwprintw(stdscr, 0, 1, "IT WAS A DRAW!");
            break;
        default:
            mvwprintw(stdscr, 0, 1, "THE GAME ENDED WITH NO WINNER.");
            break;
        }
        std::string quit_text = "PRESS 'q' TO QUIT";
        mvwprintw(stdscr, max_y-1, max_x-1-quit_text.length(), quit_text.c_str());
        attroff(COLOR_PAIR(BORDER_COLOR_PAIR));
        wrefresh(stdscr);
    }
};

class Game {
    GameWindow& game_window;
    shared_ptr<Player> player_1;
    shared_ptr<Player> player_2;
    bool game_over, play_again;
    int winner; // -1 = none, 0 = draw, 1 = player_1, 2 = player_2 etc.
    unsigned long frame_count;

    void render() {
        if (game_over) {
            game_window.renderGameOverScreen(winner);
        } else {
            game_window.render();
        }
    }

    void update() {
        CoordinatesQueue const& p1_pos = player_1->update(frame_count);
        CoordinatesQueue const& p2_pos = player_2->update(frame_count);
        winner = game_window.update(p1_pos, p2_pos);
        if (winner != -1) {
            game_over = true;
        }
    }
public:
    Game() :
        game_window(GameWindow::get_instance()),
        player_1(make_shared<Player>(1, game_window.get_player1_start(), Direction::Right, 'w', 's', 'a', 'd', game_window.get_initial_width() / 5)),
        player_2(make_shared<Player>(2, game_window.get_player2_start(), Direction::Left, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, game_window.get_initial_width() / 5)),
        game_over(false),
        play_again(false),
        winner(-1),
        frame_count(0)
    {
        game_window.set_players(player_1, player_2);
    }

    // TODO: set game_over back to false + handle new game
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)/FRAMES_PER_SECOND - time_taken_milliseconds); // run loop every 50ms
            ++frame_count;
        }
        play_again = game_window.play_again();
    }

    bool user_quit() {
        return !play_again;
    }
};
}

#endif