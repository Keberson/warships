#include <csignal>
#include <iostream>
#include <unistd.h>

#include "game.h"

#define SIGNAL_MENU 1
#define SIGNAL_START_GAME 2
#define SIGNAL_OPTIONS 3
#define SIGNAL_TITLES 4
#define SIGNAL_EXIT 5

void emergencyInterruption(int p) {
    std::cout << "\033[?25h" << std::endl;      // Only for ConsoleUI
    exit(3);
}

Game::Game() {
    _filename = STANDARD_CONFIG_PATH;
    buildGame(false);
}

Game::Game(std::string filename) {
    _filename = filename;
    buildGame(false);
}

void Game::buildGame(bool isFullBuild) {
    if (isFullBuild) {
        _rules = GameRules((_filename.empty()) ? STANDARD_CONFIG_PATH : _filename );
        ID_SHIPS_OFFSET = _rules.getNumberOfShips() / 10 + ((_rules.getNumberOfShips() % 10 == 0) ? 0 : 1) * 10;
        _players = { Player("Player1", _rules.getWidthField(), _rules.getHeightField()) };      // TODO(keberson): Задача для 3ий версии: решить, как выбирать, кто будет играть
        _computer = Computer("Computer", _rules.getWidthField(), _rules.getHeightField());
    } else {
        _ui = ConsoleUI();
        _ui.setInputMode();            // Only for ConsoleUI
        _ui.buildMenu();
    }

    signal(SIGINT, emergencyInterruption);
}


void Game::prepareToGame() {
    _ui.clearScreen();

    // TODO(keberson): Задача для 3ий версии: создание экранов для нескольких пользователей

    for (int i = _rules.getNumberOfShips() - 1; i >= 0; --i) {
        _players[0].placeShip(STANDARD_ID_START + i, _rules);
    }

    for (int i = _rules.getNumberOfShips() - 1; i >= 0; --i) {
        _computer.placeShip(STANDARD_ID_START + i, _rules);
    }
}

short Game::startGame() {
    std::string winner;
    bool isGameEnd = false;
    bool isCanTurn = true;
    _ui.clearScreen();

    while (!isGameEnd) {
        // TODO(keberson): Задача для 3ий версии: реализация для случая, если 2 игрока
        // For ConsoleUI
        _ui.displayFields(_players[0].getField(), _computer.getField());

        isCanTurn = true;

        while (isCanTurn) {
            short turnSignal = _players[0].turn(_computer.getField(), _ui);
            if (turnSignal == SIGNAL_GAME_EXIT) {
                return SIGNAL_MENU;
            }

            isCanTurn = (turnSignal == SIGNAL_TURN_HIT);

            _ui.displayFields(_players[0].getField(), _computer.getField());
            sleep(1);

            if (_computer.getField().getNumberOfHits() == _rules.getSquareOfShips()) {
                isGameEnd = true;
                break;
            }
        }

        if (isGameEnd) {
            winner = "Player";
            break;
        }

        _ui.displayFields(_players[0].getField(), _computer.getField());
        std::cout << "Computer is attacking" << std::endl;
        sleep(1);

        while (_computer.turn(_players[0].getField(), _ui)) {
            _ui.displayFields(_players[0].getField(), _computer.getField());
            std::cout << "Computer is attacking again" << std::endl;
            sleep(1);

            if (_players[0].getField().getNumberOfHits() == _rules.getSquareOfShips()) {
                isGameEnd = true;
                winner = "Computer";
                break;
            }
        }
        // /For ConsoleUI
    }

    std::cout << std::endl << "Congratulations! " << winner << " is winner!" << std::endl;
    sleep(5);
    return SIGNAL_MENU;
}

short Game::openMenu() {
    unsigned rowCounter = _ui.getMenuStartIndex();
    unsigned prevRow;
    bool isSelected = false;
    bool isEscape = false;
    bool isArrows = false;
    char c;

    _ui.clearScreen();

    while (!isSelected) {
        _ui.menuDoRowActive(rowCounter);
        _ui.displayMenu();

        c = getchar();

        if (c == '\033') {
            isEscape = true;
        }

        if (isEscape && c == '[') {
            isArrows = true;
        }

        if (isArrows) {
            prevRow = rowCounter;
            if (c == 'B') {
                if (rowCounter + 1 < _ui.getMenuSize() - 1) {
                    rowCounter++;
                } else {
                    rowCounter = _ui.getMenuStartIndex();
                }

                _ui.menuDoRowInactive(prevRow);
                _ui.menuDoRowActive(rowCounter);
            }

            if (c == 'A') {
                if (rowCounter - 1 >= _ui.getMenuStartIndex()) {
                    rowCounter--;
                } else {
                    rowCounter = _ui.getMenuSize() - 2;
                }

                _ui.menuDoRowInactive(prevRow);
                _ui.menuDoRowActive(rowCounter);
            }
        }

        if (c == '\n') {
            isSelected = true;
            _ui.menuDoRowInactive(rowCounter);
        }
    }

    rowCounter -= _ui.getMenuStartIndex();
    _ui.clearScreen();
    switch (rowCounter) {
        case 0:
            return SIGNAL_START_GAME;
        case 1:
            return SIGNAL_OPTIONS;
        case 2:
            return SIGNAL_TITLES;
        case 3:
        default:
            return SIGNAL_EXIT;
    }
}

short Game::openOptions() {
    _ui.clearScreen();
    _ui.displayOptionsMenu();           // TODO(keberson): подумать и сделать Настройки
    return SIGNAL_MENU;
}

short Game::openTitles() {
    _ui.clearScreen();
    _ui.displayTitlesMenu();
    _ui.clearScreen();
    return SIGNAL_MENU;
}

void Game::launcher() {
    bool isExit = false;
    short chapter = SIGNAL_MENU;

    while (!isExit) {
        switch (chapter) {
            case SIGNAL_MENU:
                chapter = openMenu();
                break;
            case SIGNAL_START_GAME:
                buildGame(true);
                prepareToGame();
                chapter = startGame();
                break;
            case SIGNAL_OPTIONS:
                chapter = openOptions();
                break;
            case SIGNAL_TITLES:
                chapter = openTitles();
                break;
            case SIGNAL_EXIT:
                isExit = true;
        }
    }

    _ui.clearScreen();
    std::cout << "\033[?25h" << std::endl;      // Only for ConsoleUI
    exit(1);
}
