#include "snake.h"
#include <iostream>
#include <stdexcept>

using namespace snake;

using std::cout;
using std::endl;
using std::exception;

int main() {
    Game game;
    try {
        game.start();
    } catch (exception& err) {
        cout << err.what() << endl;
        return -1;
    }
    return 0;
}