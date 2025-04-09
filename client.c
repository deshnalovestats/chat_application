#include "common.h"
#include <ncurses.h>
#include <ctype.h>

// Global Variables
WINDOW *chat_win, *input_win, *user_win;
int screen_width, screen_height;
int client_socket;

// Constants
#define MAX_HISTORY 100

// Input History
char history[MAX_HISTORY][BUFFER_SIZE];
int history_index = 0, history_count = 0;

// Function Declarations
void init_ncurses();
void cleanup_ncurses();
void *receive_messages(void *socket_desc);
void draw_borders();
void update_chat_window(const char *format, ...);
void update_user_list(const char *users[], int user_count);
void show_input_prompt();
void clear_input_window();
void clear_chat_window();
void wrap_and_display_message(WINDOW *win, const char *message, int width);
void handle_sigint(int sig);

int main() {
    struct sockaddr_in server_addr;
    pthread_t recv_thread;

    // Initialize socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Initialize ncurses
    init_ncurses();
    clear_chat_window();  // Clear chat window initially
    update_chat_window("Connected to the server.");

    // Handle SIGINT for graceful shutdown
    signal(SIGINT, handle_sigint);

    // Take username input
    char username[USERNAME_SIZE];
    clear_input_window();
    mvwprintw(input_win, 1, 1, "Enter your username: ");
    wrefresh(input_win);

    // Position the cursor and take input
    wmove(input_win, 1, 21);  // Start position for username input
    echo();                   // Enable echo for visible typing
    wgetnstr(input_win, username, USERNAME_SIZE - 1); // Limit input length
    noecho();                 // Disable echo
    username[USERNAME_SIZE - 1] = '\0'; // Ensure null-termination

    // Send the username to the server
    send(client_socket, username, strlen(username), 0);

    // Clear the input area and display a welcome message
    clear_input_window();
    update_chat_window("Welcome, %s!", username);

    // Start receiving messages
    pthread_create(&recv_thread, NULL, receive_messages, &client_socket);

    // Send messages
    char message[BUFFER_SIZE];
    int ch;
    while (1) {
        clear_input_window(); // Clear input window before taking new input
        show_input_prompt();
        wmove(input_win, 1, 21);
        wrefresh(input_win);

        echo();  // Enable echoing for user input
        int message_len = 0;
        while ((ch = wgetch(input_win)) != '\n') {
            if (ch == KEY_UP) {
                // Recall previous message
                if (history_count > 0) {
                    history_index = (history_index - 1 + history_count) % history_count;
                    strncpy(message, history[history_index], BUFFER_SIZE);
                    message_len = strlen(message);
                    clear_input_window();
                    mvwprintw(input_win, 1, 21, "%s", message);
                    wmove(input_win, 1, 21 + message_len);
                    wrefresh(input_win);
                }
            } else if (ch == KEY_DOWN) {
                // Recall next message
                if (history_count > 0) {
                    history_index = (history_index + 1) % history_count;
                    strncpy(message, history[history_index], BUFFER_SIZE);
                    message_len = strlen(message);
                    clear_input_window();
                    mvwprintw(input_win, 1, 21, "%s", message);
                    wmove(input_win, 1, 21 + message_len);
                    wrefresh(input_win);
                }
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (message_len > 0) {
                    message[--message_len] = '\0';
                    mvwdelch(input_win, 1, 21 + message_len);
                    wrefresh(input_win);
                }
            } else if (isprint(ch)) {
                if (message_len < BUFFER_SIZE - 1) {
                    message[message_len++] = ch;
                    mvwaddch(input_win, 1, 21 + message_len - 1, ch);
                    wrefresh(input_win);
                }
            }
        }
        message[message_len] = '\0';
        noecho();  // Disable echoing after input

        // Handle empty messages or unintended inputs
        if (strlen(message) == 0) {
            continue;  // Ignore empty inputs or errors
        }

        // If the user types '/quit', exit the loop
        if (strcmp(message, "/quit") == 0) {
            update_chat_window("You have left the chat.");
            break;
        }

        // Save the valid message to history
        strncpy(history[history_count % MAX_HISTORY], message, BUFFER_SIZE);
        history_count++;
        history_index = history_count;

        // Send message to server
        send(client_socket, message, strlen(message), 0);

        // Display the message in the chat window
        char formatted_message[MSG_SIZE];
        snprintf(formatted_message, sizeof(formatted_message), "[me]: %s", message);
        update_chat_window("%s", formatted_message);
    }

    close(client_socket);
    cleanup_ncurses();
    return 0;
}

// Initialize ncurses and create windows
void init_ncurses() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);  // Enable keypad for capturing arrow keys
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);  // Chat window
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // Input prompt
    init_pair(3, COLOR_RED, COLOR_BLACK);   // Errors
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Usernames

    getmaxyx(stdscr, screen_height, screen_width);

    chat_win = newwin(screen_height - 3, screen_width * 3 / 4, 0, 0);
    input_win = newwin(3, screen_width, screen_height - 3, 0);
    user_win = newwin(screen_height - 3, screen_width / 4, 0, screen_width * 3 / 4);

    scrollok(chat_win, TRUE);
    draw_borders();
    refresh();
}

// Cleanup ncurses
void cleanup_ncurses() {
    delwin(chat_win);
    delwin(input_win);
    delwin(user_win);
    endwin();
}

// Draw window borders
void draw_borders() {
    box(chat_win, 0, 0);
    box(input_win, 0, 0);
    box(user_win, 0, 0);
    mvwprintw(user_win, 0, 1, " Active Users ");
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(user_win);
}

// Clear the input window
void clear_input_window() {
    werase(input_win);         // Erase input window contents
    box(input_win, 0, 0);      // Redraw the box around the input window
    wrefresh(input_win);       // Refresh the input window
}

// Clear the chat window
void clear_chat_window() {
    werase(chat_win);          // Erase chat window contents
    box(chat_win, 0, 0);       // Redraw the box around the chat window
    wrefresh(chat_win);        // Refresh the chat window
}

// Show the input prompt
void show_input_prompt() {
    wattron(input_win, COLOR_PAIR(2) | A_ITALIC);
    mvwprintw(input_win, 1, 1, "Type to send a text: ");
    wattroff(input_win, COLOR_PAIR(2) | A_ITALIC);
    wrefresh(input_win);
}

// Update the chat window
void update_chat_window(const char *format, ...) {
    va_list args;
    char buffer[BUFFER_SIZE];

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    wrap_and_display_message(chat_win, buffer, screen_width * 3 / 4 - 2);
    wrefresh(chat_win);
}

// Wrap and display a message
void wrap_and_display_message(WINDOW *win, const char *message, int width) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, message, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    for (int i = 0; i < strlen(buffer); i += width) {
        char line[width + 1];
        strncpy(line, &buffer[i], width);
        line[width] = '\0';
        wprintw(win, "%s\n", line);
    }
    wrefresh(win);
}

// Update user list
void update_user_list(const char *users[], int user_count) {
    werase(user_win);
    box(user_win, 0, 0);
    mvwprintw(user_win, 0, 1, " Active Users ");
    for (int i = 0; i < user_count; ++i) {
        mvwprintw(user_win, i + 1, 1, "%s", users[i]);
    }
    wrefresh(user_win);
}

// Receive messages
void *receive_messages(void *socket_desc) {
    int client_socket = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            update_chat_window("Disconnected from the server.");
            break;
        }

        buffer[bytes_read] = '\0';
        update_chat_window("%s", buffer);
    }

    return NULL;
}

// Handle SIGINT for graceful shutdown
void handle_sigint(int sig) {
    printf("\nShutting down client...\n");

    // Close client socket
    close(client_socket);

    // Cleanup ncurses
    cleanup_ncurses();
    exit(0);
}