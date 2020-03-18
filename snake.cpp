#include "snake.h"
#include <exception>

using namespace snake;

using std::exception;

int main() {
    try {
        Game game;
        game.play();
    } catch (const exception&) {
        return -1;
    }
    return 0;
}