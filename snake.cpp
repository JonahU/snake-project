#include <deque>
#include <ncurses.h>

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

    void update() {
        my_snake.move();
    }

    int who() {
        return identifier;
    }
};

int main() {
    Player player_1{1, {25,50}, Direction::Right, 'w', 's', 'a', 'd'};
    Player player_2{2, {75,50}, Direction::Left, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
}