// Midnight Breach - Choose Your Own Adventure (CSC 134)
// You are a security analyst. An intruder is stealing data from the company
// network. Collect the laptop, the admin token, and the firewall disk, then
// lock the intruder out from the Server Room before you run out of moves.
//
// This version combines Midnight Breach with the M6BONUS1 menu program.
// AI assistance: Claude (Anthropic).
//
// Concepts used: arrays (1D, parallel, 2D), loops (while/for), if/else,
// functions, enums, constants, strings, input validation, ASCII graphics,
// an adjacency list (exits[room][direction]), and numbered menus.
// Regular arrays only (no vectors).
//
// MENUS: every menu has at least 10 numbered options.
//   - Option 0 is ALWAYS "EXIT", so you can quit from any menu.
//   - Item menus list all 14 items plus BACK and EXIT.
//   - Menus are printed HORIZONTALLY (several options per row).
//
// GRAPHICS: colors, a screen clear, and a "data stolen" progress bar, all
// done with ANSI escape codes (no extra libraries needed).

#include <iostream>
#include <string>
#include <limits>      // numeric_limits, used to clear bad input
#include <iomanip>     // setw, used to line up the horizontal menus

/*
   windows.h comes with every Windows C++ compiler, so it is not an extra
   library. We only need it to switch on color support in the Windows
   console (see enableColors below). The #ifdef means Mac/Linux skip it.

   NOMINMAX stops windows.h from defining its own min and max, which would
   break numeric_limits<streamsize>::max() in getChoice().
*/
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

using namespace std;

/*
   ---------- ANSI color codes ----------
   An ANSI escape code is a special string that starts with "\033[" .
   The terminal does not print it; instead it changes how the text after
   it looks. RESET turns everything back to normal, so always print RESET
   after colored text or the color will "leak" onto later lines.
*/
const string RESET   = "\033[0m";
const string BOLD    = "\033[1m";
const string DIM     = "\033[2m";
const string RED     = "\033[91m";
const string GREEN   = "\033[92m";
const string YELLOW  = "\033[93m";
const string BLUE    = "\033[94m";
const string MAGENTA = "\033[95m";
const string CYAN    = "\033[96m";
const string WHITE   = "\033[97m";
const string GRAY    = "\033[90m";
const string CLEAR_SCREEN = "\033[2J\033[H";   // wipe screen, cursor to top

// ---------- Constants and enums ----------
enum Direction { NORTH, EAST, SOUTH, WEST, NUM_DIRECTIONS };

enum Room { SECURITY_OFFICE, SERVER_ROOM, NETWORK_CLOSET, BREAK_ROOM,
            ARCHIVE, EXEC_OFFICE, LOBBY, GARAGE, ROOF, VAULT,
            CONF_ROOM, HELP_DESK, NUM_ROOMS };                    // 12 rooms

enum Item { BADGE, LAPTOP, TOKEN, FIREWALL_DISK, VAULT_KEY, PRINTOUT,
            USB_DRIVE, FLASHLIGHT, HEADSET, COFFEE, ENERGY_DRINK,
            SKATEBOARD, MAP, STICKY_NOTE, NUM_ITEMS };            // 14 items

const int GRID_ROWS = 3;
const int GRID_COLS = 4;
const int IN_INVENTORY = -2;   // item is being carried
const int GONE = -3;           // item was used up
const int NO_EXIT = -1;        // no door in that direction
const int START_MOVES = 40;    // moves before the breach completes

// Main menu option numbers (so the code reads like words, not numbers)
const int MENU_EXIT = 0;
const int MENU_NORTH = 1;
const int MENU_EAST = 2;
const int MENU_SOUTH = 3;
const int MENU_WEST = 4;
const int MENU_LOOK = 5;
const int MENU_TAKE = 6;
const int MENU_USE = 7;
const int MENU_INVENTORY = 8;
const int MENU_EXAMINE = 9;
const int MENU_MAP = 10;
const int MENU_HELP = 11;
const int MENU_HINT = 12;
const int MAIN_MENU_MAX = 12;

// Item sub-menu: options 1..14 pick an item, 15 goes back, 0 exits
const int ITEM_MENU_BACK = NUM_ITEMS + 1;   // = 15

// Values returned by pickItem() when the player does not choose an item
const int PICK_BACK = -1;
const int PICK_EXIT = -2;

const string DIR_NAMES[NUM_DIRECTIONS] = {"north", "east", "south", "west"};

// ---------- ASCII pictures (one per room, same order as the Room enum) ----------
// Each picture is a "raw string" R"ART( ... )ART" so backslashes and quotes
// print exactly as drawn, with no escape characters needed.
const string ROOM_ART[NUM_ROOMS] = {
// SECURITY_OFFICE
R"ART(   .-----------.  .-----------.  .-----------.
   | ! ALERT ! |  | > login:  |  | CAM 03    |
   | ######### |  | > root ?? |  |   [ o ]   |
   | UNAUTH'D  |  | > DENIED_ |  |    /|\    |
   '-----.-----'  '-----.-----'  '-----.-----'
   ______|______________|______________|______
  |   [::::::::::]    c[_]    (_)   [phone]  |
  |__________________________________________|
     ||                                  ||)ART",

// SERVER_ROOM
R"ART(   _________   _________   _________
  | ::::::: | | ::::::: | | ::::::: |
  | o o o o | | o o o o | | o * o * |==.
  | ------- | | ------- | | ------- |  |
  | o o o o | | o o o o | | * o * o |  |
  | ::::::: | | ::::::: | | ::::::: |  |
  |_________| |_________| |_________|  |
                                       '==> 1011 >> 0110 >> ???
                    >>> D A T A   L E A K >>>)ART",

// NETWORK_CLOSET
R"ART(   ________________________________     _________
  |[]|[]|[]|[]|[]|[]|[]|[]|[]|[]|[]|   |  _____  |
  |================================|   | |     | |
  | o  o  o  o  o  o  o  o  o  o  o|   | |  O--| |
  |  \  \  |  /  /  \  \  |  /  /  |   | |_____| |
  |   ~~~~~~~~ CABLES ~~~~~~~~~    |   |_________|
  |________________________________|     (safe is
          S W I T C H E S                  open!))ART",

// BREAK_ROOM
R"ART(   .-----------.              (   )   (  )
   |  S O D A  |               ) (     ) (
   |  [=] [=]  |           .---------------.
   |  [=] [=]  |           |  c[_]   c[_]  |
   |  [=] [=]  |           '---------------'
   |   _____   |              ||       ||
   |  |_____|  |              ||       ||
   '-----------'          ~ hummmmmmmm ~)ART",

// ARCHIVE
R"ART(   _________________________________
  | [BOX] [BOX]  [BOX] [BOX]  [BOX] |
  |=================================|
  | [||||] [||||]   [1998]  [2003]  |
  |=================================|
  | [BOX]   [old CRT]   [BOX] [BOX] |
  |_________________________________|
     .   dust   .     .    dust  .)ART",

// EXEC_OFFICE
R"ART(      ______________________________
     |  __________________________  |
     | |   CEO-PC   [LOGGED IN]   | |
     | |   user: ceo   pw: ****   | |
     | |__________________________| |
     |______________[__]____________|
      ______________||______________
     |______________________________|
            .-------.
            | CHAIR |   (still warm...)
            '-------')ART",

// LOBBY
R"ART(   _________________________________________
  |   ~~  W E L C O M E  ~~       .-----.   |
  |                               |12:00|   |
  |    ______________________     '-----'   |
  |   |   F R O N T  D E S K |    _____     |
  |   |______________________|   | | | |    |
  |______________________________|_|_|_|____|
              (nobody here)        doors)ART",

// GARAGE
R"ART(  .   .   .   .   .   .   .   .   .   .   .
      ________            ________
    _/   ||   \__       _/   ||   \__
   |  _        _ |     |  _        _ |    [P2]
   '-(_)------(_)'     '-(_)------(_)'
  .   .   .   .   .   .   .   .   .   .   .
       * flicker *            * flicker *)ART",

// ROOF
R"ART(            \ | /                    .--.
         --  ( O )  --              (    )  <- dish
            / | \                    '--'\
       |      |      |                    \
      -+-     |     -+-                    |
   ____|______|______|_____________________|___
  |       R  O  O  F        ~ ~ windy ~ ~      |)ART",

// VAULT
R"ART(   _______________________________
  |  ___________________________  |
  | |        .---------.        | |
  | |       /  .-'-'-.  \   [1] | |
  | |      |  (  -+-  )  |  [2] | |
  | |       \  '-.-.-'  /   [3] | |
  | |        '---------'        | |
  | |___________________________| |
  |_______________________________|
            V   A   U   L   T)ART",

// CONF_ROOM
R"ART(     o    o    o    o    o    o
   __|____|____|____|____|____|__      .------.
  |                              |     | /\_  |
  |   ( coffee rings )   c[_]    |     |/   \_|
  |______________________________|     '------'
     |    |    |    |    |    |           ||
     o    o    o    o    o    o     (chairs pushed back))ART",

// HELP_DESK
R"ART(   ______________________________________
  |  H E L P   D E S K   [ TICKETS: 47 ] |
  |______________________________________|
       _n_          _n_          _n_
      (o o)        (   )        (o o)
   ___[___]________[___]________[___]___
  |    #4521        #4522        #4523  |
  |_____________________________________|)ART"
};

const string TITLE_ART = R"ART(
 ===================================================
     __  ___________  _   ______________  ________
    /  |/  /  _/ __ \/ | / /  _/ ____/ / / /_  __/
   / /|_/ // // / / /  |/ // // / __/ /_/ / / /
  / /  / // // /_/ / /|  // // /_/ / __  / / /
 /_/  /_/___/_____/_/ |_/___/\____/_/ /_/ /_/
            ____  ____  _________   ________  __
           / __ )/ __ \/ ____/   | / ____/ / / /
          / __  / /_/ / __/ / /| |/ /   / /_/ /
         / /_/ / _, _/ /___/ ___ / /___/ __  /
        /_____/_/ |_/_____/_/  |_\____/_/ /_/
 ===================================================
   01001101 01001001 01000100  >> 12:00 AM >> )ART";

const string WIN_ART = R"ART(
          _____________
         |  _________  |
         | |       / | |      ==============================
         | |      /  | |        S Y S T E M    S E C U R E
         | | \   /   | |        Intruder locked out!
         | |  \_/    | |      ==============================
          \ \_______/ /
           \_________/             \o/
                                    |    Great work, analyst!
                                   / \)ART";

const string LOSE_ART = R"ART(
            _________
          .'         '.
         /   _     _   \
        |   (x)   (x)   |
        |       ^       |
         \    '---'    /
          '.__|||||__.'
            |_______|
   10110 >> 01101 >> 11010 >>   [##########] 100%
      ===================================
         D A T A   S T O L E N
         The breach is complete.
      ===================================)ART";

/*
   A color for each room's picture. This is another parallel array:
   ROOM_COLOR[i] goes with ROOM_ART[i], in the same order as the Room enum.
   The Garage is GRAY because it is dark in there.
*/
const string ROOM_COLOR[NUM_ROOMS] = {
    RED,      // Security Office - alarms
    CYAN,     // Server Room
    GREEN,    // Network Closet
    YELLOW,   // Break Room
    WHITE,    // Archive
    MAGENTA,  // Executive Office
    BLUE,     // Lobby
    GRAY,     // Parking Garage - dark
    CYAN,     // Roof
    YELLOW,   // Vault - gold
    BLUE,     // Conference Room
    GREEN     // Help Desk
};

// One-line icon for each item (same order as the Item enum)
const string ITEM_ICON[NUM_ITEMS] = {
    "[ID|badge]", "[=laptop=]", "(token)", "(@ disk @)", "-o|key",
    "[~mail~]", "[usb]=", "(|flashlight>", "(headset)", "c[_]coffee",
    "[energy!]", "o==skate==o", "[~map~]", "[note*]"
};

// ---------- Functions ----------

/*
   Turn on ANSI color support in the Windows console.
   Newer Windows consoles understand ANSI codes, but only after the program
   asks for it with SetConsoleMode. On Mac/Linux this function does nothing
   because the #ifdef skips the code inside.
*/
void enableColors() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/*
   Draw the "data stolen" progress bar.
   The fewer moves you have left, the more data the intruder has copied.
       filled = how many of the 20 boxes to fill in
   Coffee and energy drinks can push movesLeft ABOVE START_MOVES, which
   would make 'filled' negative, so we clamp it between 0 and BAR_WIDTH.
   The bar changes color as it fills: green, then yellow, then red.
*/
void showProgressBar(int movesLeft) {
    const int BAR_WIDTH = 20;
    int filled = (START_MOVES - movesLeft) * BAR_WIDTH / START_MOVES;
    if (filled < 0) filled = 0;
    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    int percent = filled * 100 / BAR_WIDTH;

    string color = GREEN;
    if (percent >= 75)      color = RED;
    else if (percent >= 40) color = YELLOW;

    cout << "Data stolen: [" << color << string(filled, '#') << RESET
         << GRAY << string(BAR_WIDTH - filled, '-') << RESET << "] "
         << color << percent << "%" << RESET << endl;
}

// Ask the player for a number between 0 and 'high'.
// Keeps asking until the input is valid. Returns 0 (Exit) if input ends,
// so the game can never get stuck in an infinite loop.
int getChoice(int high) {
    int choice;
    while (true) {
        cout << "Enter choice (0 to exit): ";
        if (!(cin >> choice)) {
            if (cin.eof()) return 0;                 // no more input: exit
            cin.clear();                             // reset the error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  // throw away bad text
            cout << "Please enter a number." << endl;
        } else if (choice < 0 || choice > high) {
            cout << "Please choose a number from 0 to " << high << "." << endl;
        } else {
            return choice;
        }
    }
}

// Print one menu option as "number. label", padded to a fixed width so the
// options line up in neat columns across the page.
void printOption(int number, const string label, int width) {
    cout << left << setw(width) << (to_string(number) + ". " + label);
}

// Print the main menu HORIZONTALLY (13 options including Exit).
void showMainMenu() {
    cout << "\n" << CYAN << BOLD << "===== MAIN MENU =====" << RESET << endl;
    // Row 1: movement
    printOption(MENU_NORTH, "North", 16);
    printOption(MENU_EAST, "East", 16);
    printOption(MENU_SOUTH, "South", 16);
    printOption(MENU_WEST, "West", 16);
    printOption(MENU_LOOK, "Look", 16);
    cout << endl;
    // Row 2: actions with items
    printOption(MENU_TAKE, "Take", 16);
    printOption(MENU_USE, "Use", 16);
    printOption(MENU_INVENTORY, "Inventory", 16);
    printOption(MENU_EXAMINE, "Examine", 16);
    printOption(MENU_MAP, "Map", 16);
    cout << endl;
    // Row 3: help and exit
    printOption(MENU_HELP, "Help", 16);
    printOption(MENU_HINT, "Hint", 16);
    printOption(MENU_EXIT, "EXIT", 16);
    cout << endl;
}

// Print an item sub-menu HORIZONTALLY: all 14 items, then Back (15) and Exit (0).
// 'title' says what the menu is for (Take / Use / Examine).
// Items are printed 4 per row; a loop starts a new row after every 4th option.
void showItemMenu(const string title, const string names[]) {
    cout << "\n" << CYAN << BOLD << "===== " << title << " - WHICH ITEM? =====" << RESET << endl;
    for (int i = 0; i < NUM_ITEMS; i++) {
        printOption(i + 1, names[i], 22);
        if ((i + 1) % 4 == 0) cout << endl;      // end the row after 4 options
    }
    printOption(ITEM_MENU_BACK, "BACK", 22);     // 15 = back to main menu
    printOption(MENU_EXIT, "EXIT", 22);          // 0 = exit game
    cout << endl;
}

// Show an item menu and get the player's pick.
// Returns the item index (0..NUM_ITEMS-1), PICK_BACK, or PICK_EXIT.
int pickItem(const string title, const string names[]) {
    showItemMenu(title, names);
    int pick = getChoice(ITEM_MENU_BACK);
    if (pick == 0) return PICK_EXIT;
    if (pick == ITEM_MENU_BACK) return PICK_BACK;
    return pick - 1;                             // menu number -> array index
}

// Draw the whole building from the grid. * marks the player.
void showMap(const string roomName[], const int grid[GRID_ROWS][GRID_COLS], int current) {
    cout << "\n" << CYAN << BOLD << "=== BUILDING MAP (* = you) ===" << RESET << endl;
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int room = grid[r][c];
            cout << (room == current ? YELLOW + "*" + RESET : string(" ")) << "["
                 << left << setw(16) << roomName[room] << "]-";
        }
        cout << endl;
        if (r < GRID_ROWS - 1) {                 // vertical connectors
            for (int c = 0; c < GRID_COLS; c++)
                cout << string(10, ' ') << "|" << string(9, ' ');
            cout << endl;
        }
    }
    cout << "(Closet needs the badge. Vault needs the vault key.)" << endl;
}

int main() {
    // ---------- Parallel arrays: rooms (index with the Room enum) ----------
    string roomName[NUM_ROOMS] = {
        "Security Office", "Server Room", "Network Closet", "Break Room",
        "Archive", "Executive Office", "Lobby", "Parking Garage",
        "Roof", "Vault", "Conference Room", "Help Desk"
    };
    string roomDesc[NUM_ROOMS] = {
        "Monitors flash red: UNAUTHORIZED LOGIN DETECTED.",
        "Rows of servers. One terminal is streaming data to an unknown address.",
        "A cramped closet of switches and cables. A safe sits open.",
        "A quiet break room. A vending machine hums in the corner.",
        "Dusty shelves of old records and unused equipment.",
        "A plush office. The CEO's computer is still logged in.",
        "A marble lobby. The front desk is empty.",
        "A concrete garage with flickering lights.",
        "A windy rooftop covered in antennas and satellite dishes.",
        "A steel vault. Backup disks sit in numbered slots.",
        "A long table with chairs pushed back in a hurry.",
        "Headsets and half-finished tickets cover the desks."
    };

    // ---------- Parallel arrays: items (index with the Item enum) ----------
    string itemName[NUM_ITEMS] = {
        "access badge", "laptop", "admin token", "firewall disk",
        "vault key", "phishing printout", "usb drive", "flashlight",
        "headset", "coffee", "energy drink", "skateboard", "building map",
        "sticky note"
    };
    string itemDesc[NUM_ITEMS] = {
        "A badge marked 'NETWORK CLOSET ACCESS'.",
        "A security laptop with incident-response tools installed.",
        "A hardware token that holds the admin lockout key.",
        "A disk labeled 'FIREWALL RULES - RESTORE'.",
        "A heavy steel key stamped 'VAULT'.",
        "A printed email: 'Urgent! Reset your password at this link.'",
        "A USB drive that can save the server activity logs.",
        "A heavy flashlight with fresh batteries.",
        "A headset. Faint IT chatter plays in the earpiece.",
        "A hot coffee. It might buy you some time.",
        "A cold energy drink. Big boost, short crash.",
        "A skateboard. Every other move is free while you carry it.",
        "A folded map of the whole building.",
        "A sticky note in messy handwriting."
    };
    // What happens when you use each item (special cases are handled in the code)
    string itemUseText[NUM_ITEMS] = {
        "You wave the badge around. It only works on the closet door.",
        "The laptop needs the Server Room, a token, and a firewall disk.",
        "The token needs a laptop to plug into.",
        "The disk needs a laptop to load it.",
        "You turn the key over. It must fit the vault door.",
        "The email came from the CEO's account. The attacker has CEO credentials.",
        "You turn the USB drive over. Try the Server Room.",
        "You click on the flashlight. The shadows retreat.",
        "IT chatter: 'The firewall disk is locked in the Vault.'",
        "You sip the coffee. Slightly better.",
        "You pop the can. Zoom!",
        "Just keep it with you. Every other move will be free.",
        "You unfold the map.",
        "Note: 'Vault key is in the Parking Garage. Bring a light!'"
    };
    // Where each item starts: a room number, IN_INVENTORY, or GONE
    int itemLoc[NUM_ITEMS] = {
        ARCHIVE, SECURITY_OFFICE, NETWORK_CLOSET, VAULT,
        GARAGE, BREAK_ROOM, EXEC_OFFICE, HELP_DESK,
        HELP_DESK, BREAK_ROOM, ROOF, LOBBY, CONF_ROOM,
        CONF_ROOM
    };

    // ---------- 2D array: the building layout (3 rows x 4 columns) ----------
    const int grid[GRID_ROWS][GRID_COLS] = {
        { ARCHIVE,   EXEC_OFFICE,     CONF_ROOM,   ROOF           },
        { HELP_DESK, SECURITY_OFFICE, SERVER_ROOM, NETWORK_CLOSET },
        { GARAGE,    LOBBY,           BREAK_ROOM,  VAULT          }
    };

    // ---------- Adjacency list (2D array) built from the grid ----------
    // exits[from_room][direction] = to_room, or NO_EXIT (-1)
    int exits[NUM_ROOMS][NUM_DIRECTIONS];
    for (int r = 0; r < NUM_ROOMS; r++)
        for (int d = 0; d < NUM_DIRECTIONS; d++)
            exits[r][d] = NO_EXIT;

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int room = grid[r][c];
            if (r > 0)             exits[room][NORTH] = grid[r - 1][c];
            if (c < GRID_COLS - 1) exits[room][EAST]  = grid[r][c + 1];
            if (r < GRID_ROWS - 1) exits[room][SOUTH] = grid[r + 1][c];
            if (c > 0)             exits[room][WEST]  = grid[r][c - 1];
        }
    }

    // ---------- Game state ----------
    int current = SECURITY_OFFICE;
    int movesLeft = START_MOVES;
    int stepCount = 0;                // counts moves (used by the skateboard)
    int score = 0;
    bool logsSaved = false;
    bool playing = true, won = false, newRoom = true;
    bool freeMove = false;            // true right after a free skateboard move

    enableColors();                   // needed for colors on Windows

    cout << CLEAR_SCREEN;
    cout << CYAN << BOLD << TITLE_ART << RESET << endl;
    cout << "\nAn intruder is stealing company data. Stop them before time runs out!" << endl;
    cout << "Goal: get the laptop, admin token, and firewall disk to the Server Room." << endl;

    /*
       Wait for Enter before starting. Without this pause, the screen clear
       at the start of the game loop would wipe the title right away.
    */
    cout << "\n" << YELLOW << "Press Enter to begin..." << RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // ---------- Main game loop ----------
    while (playing && movesLeft > 0) {
        bool hasLight = (itemLoc[FLASHLIGHT] == IN_INVENTORY);

        /*
           When the player walks into a new room, clear the screen so each
           room feels like its own "page" instead of one long scroll.
           Other actions (Take, Inventory, etc.) do NOT clear, so their
           messages stay on screen until the player moves.
        */
        if (newRoom) cout << CLEAR_SCREEN;

        cout << "\n" << BOLD << WHITE << "--- " << roomName[current] << RESET
             << " (moves left: " << movesLeft << " | score: " << score << ") ---" << endl;
        showProgressBar(movesLeft);
        if (newRoom) {                               // draw the picture on entry
            cout << ROOM_COLOR[current] << ROOM_ART[current] << RESET << endl;
            newRoom = false;
        }
        if (freeMove) {                              // shown after the clear so it isn't wiped
            cout << GREEN << "You glide on the skateboard. That move was free!" << RESET << endl;
            freeMove = false;
        }
        cout << roomDesc[current] << endl;

        if (current == GARAGE && !hasLight) {
            cout << GRAY << "It is pitch dark. You can't see any items without a light." << RESET << endl;
        } else {
            for (int i = 0; i < NUM_ITEMS; i++)      // show items in this room
                if (itemLoc[i] == current)
                    cout << YELLOW << "You see: " << itemName[i] << "  " << ITEM_ICON[i] << RESET << endl;
        }

        cout << "Exits:";
        bool hasExits = false;
        for (int d = 0; d < NUM_DIRECTIONS; d++)     // show available exits
            if (exits[current][d] != NO_EXIT) {
                cout << " " << GREEN << DIR_NAMES[d] << RESET;
                hasExits = true;
            }
        if (!hasExits) cout << " none";
        cout << endl;

        if (movesLeft <= 5)
            cout << RED << BOLD << "!! CRITICAL: transfer 95% complete !!" << RESET << endl;
        else if (movesLeft <= 12)
            cout << YELLOW << "WARNING: intruder is copying more data..." << RESET << endl;

        // Show the menu and get the player's choice
        showMainMenu();
        int choice = getChoice(MAIN_MENU_MAX);

        if (choice == MENU_EXIT) {                            // exit
            playing = false;

        } else if (choice >= MENU_NORTH && choice <= MENU_WEST) {   // movement
            int next = exits[current][choice - MENU_NORTH];   // look up the adjacency list
            if (next == NO_EXIT) {
                cout << RED << "You can't go that way." << RESET << endl;
            } else if (next == NETWORK_CLOSET && itemLoc[BADGE] != IN_INVENTORY) {
                cout << RED << "The closet door is locked. It needs an access badge." << RESET << endl;
            } else if (next == VAULT && itemLoc[VAULT_KEY] != IN_INVENTORY) {
                cout << RED << "The vault is locked. It needs a vault key." << RESET << endl;
            } else {
                current = next;
                newRoom = true;
                stepCount++;
                if (itemLoc[SKATEBOARD] == IN_INVENTORY && stepCount % 2 == 0)
                    freeMove = true;                  // message prints after the screen clears
                else
                    movesLeft--;
            }

        } else if (choice == MENU_LOOK) {                     // look
            cout << ROOM_COLOR[current] << ROOM_ART[current] << RESET << endl;

        } else if (choice == MENU_TAKE) {                     // take
            int item = pickItem("TAKE", itemName);
            if (item == PICK_EXIT) {
                playing = false;
            } else if (item >= 0) {
                if (current == GARAGE && !hasLight) {
                    cout << "It's too dark to find anything." << endl;
                } else if (itemLoc[item] == current) {
                    itemLoc[item] = IN_INVENTORY;
                    score += 5;
                    cout << GREEN << "Taken: " << itemName[item] << "  " << ITEM_ICON[item] << RESET << endl;
                } else {
                    cout << "You don't see that here." << endl;
                }
            }

        } else if (choice == MENU_USE) {                      // use
            int item = pickItem("USE", itemName);
            if (item == PICK_EXIT) {
                playing = false;
            } else if (item >= 0) {
                if (itemLoc[item] != IN_INVENTORY) {
                    cout << "You aren't carrying that." << endl;
                } else if (item == LAPTOP) {
                    // Winning move: needs the Server Room, token, and disk
                    if (current != SERVER_ROOM) {
                        cout << "The laptop shows 'No secure connection'. Try the Server Room." << endl;
                    } else if (itemLoc[TOKEN] != IN_INVENTORY) {
                        cout << "The laptop asks for an admin token." << endl;
                    } else if (itemLoc[FIREWALL_DISK] != IN_INVENTORY) {
                        cout << "The laptop asks for the firewall disk." << endl;
                    } else {
                        cout << "You plug in the token, load the firewall disk," << endl;
                        cout << "and run the lockout script..." << endl;
                        score += 50;
                        won = true;
                        playing = false;
                    }
                } else if (item == USB_DRIVE) {
                    if (current == SERVER_ROOM && !logsSaved) {
                        logsSaved = true;
                        score += 20;
                        cout << GREEN << "You save the server logs as evidence. +20 points." << RESET << endl;
                    } else {
                        cout << "Nothing to copy here. Try the Server Room." << endl;
                    }
                } else if (item == COFFEE) {
                    itemLoc[COFFEE] = GONE;
                    movesLeft += 3;
                    cout << GREEN << "You feel sharper. +3 moves." << RESET << endl;
                } else if (item == ENERGY_DRINK) {
                    itemLoc[ENERGY_DRINK] = GONE;
                    movesLeft += 5;
                    cout << GREEN << "Zoom! +5 moves." << RESET << endl;
                } else if (item == MAP) {
                    showMap(roomName, grid, current);
                } else {
                    cout << itemUseText[item] << endl;        // normal use message
                }
            }

        } else if (choice == MENU_INVENTORY) {                // inventory
            cout << "You carry:" << endl;
            bool any = false;
            for (int i = 0; i < NUM_ITEMS; i++)
                if (itemLoc[i] == IN_INVENTORY) {
                    cout << "  " << left << setw(20) << itemName[i] << ITEM_ICON[i] << endl;
                    any = true;
                }
            if (!any) cout << "  nothing" << endl;

        } else if (choice == MENU_EXAMINE) {                  // examine
            int item = pickItem("EXAMINE", itemName);
            if (item == PICK_EXIT) {
                playing = false;
            } else if (item >= 0) {
                if (itemLoc[item] == IN_INVENTORY || itemLoc[item] == current)
                    cout << ITEM_ICON[item] << "  " << itemDesc[item] << endl;
                else
                    cout << "You don't see that here." << endl;
            }

        } else if (choice == MENU_MAP) {                      // map (always available)
            showMap(roomName, grid, current);

        } else if (choice == MENU_HELP) {                     // help
            cout << "Type the number of a menu option and press Enter." << endl;
            cout << "Option 0 exits the game from any menu." << endl;
            cout << "Goal: get the laptop, admin token, and firewall disk," << endl;
            cout << "then use the laptop in the Server Room before moves run out." << endl;
            cout << "Taking items scores points; saving moves gives a bonus." << endl;

        } else if (choice == MENU_HINT) {                     // hint (changes with progress)
            if (itemLoc[LAPTOP] != IN_INVENTORY)
                cout << MAGENTA << "Hint: you need the laptop from the Security Office." << RESET << endl;
            else if (itemLoc[TOKEN] != IN_INVENTORY && itemLoc[BADGE] != IN_INVENTORY)
                cout << MAGENTA << "Hint: find an access badge in the Archive (northwest corner)." << RESET << endl;
            else if (itemLoc[TOKEN] != IN_INVENTORY)
                cout << MAGENTA << "Hint: the token is in the Network Closet, east of the Server Room." << RESET << endl;
            else if (itemLoc[FIREWALL_DISK] != IN_INVENTORY && itemLoc[VAULT_KEY] != IN_INVENTORY)
                cout << MAGENTA << "Hint: the firewall disk is in the Vault (southeast corner)." << RESET << endl;
            else if (itemLoc[FIREWALL_DISK] != IN_INVENTORY && itemLoc[FLASHLIGHT] != IN_INVENTORY)
                cout << MAGENTA << "Hint: the Garage is dark. Get a flashlight from the Help Desk." << RESET << endl;
            else if (itemLoc[FIREWALL_DISK] != IN_INVENTORY)
                cout << MAGENTA << "Hint: the vault key is in the Parking Garage (southwest corner)." << RESET << endl;
            else
                cout << MAGENTA << "Hint: use the laptop in the Server Room." << RESET << endl;
        }
    }

    // ---------- Ending ----------
    if (won) {
        cout << GREEN << BOLD << WIN_ART << RESET << endl;
        int bonus = movesLeft * 2;                   // reward saving moves
        score += bonus;
        cout << "\nMoves left: " << movesLeft << " (bonus +" << bonus << ")" << endl;
        string rank = "Junior Analyst";
        if (movesLeft >= 20)      rank = "Elite Analyst";
        else if (movesLeft >= 10) rank = "Senior Analyst";
        cout << "Rank: " << YELLOW << BOLD << rank << RESET << endl;
    } else if (movesLeft <= 0 && playing) {
        cout << RED << BOLD << LOSE_ART << RESET << endl;
    }
    cout << "Final score: " << BOLD << score << RESET << endl;
    cout << "Thanks for playing!" << endl;
    return 0;
}