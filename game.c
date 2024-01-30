// ^_^Ali^_^
// project Othello: game.c

/*
Note1:
    map[i][j] -> i moves on x axis, j moves on y axis
    so i and j are not defind like in martixs, the are defined like in vectors (physics)
Note2:
    cm: continuing match / fm: finished match
    sortedname0, sortedname1: sorted of (player1name, player2name)
    cm-sortedname0-sortedname1-NO
    for example: cm-ali-mmd-1, cm-ali-mmd-2, ...
Note3:
    the way of saving inside a file:
    time_game (0/1)
    p1.name p1.score [p1.remaining_time p1.undo_moves_left, if time_game]
    p2.name p2.score [p2.remaining_time p2.undo_moves_left, if time_game]
    winner (0 -> not yet / 1 -> player1 / 2 -> player2 / 3 -> draw!)
    turn (1/2)
    <map> (8 lines of 8 one-digits numbers (represending the value of the disc in the position), seprated each by one space)
*/



//#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#include "encryption.c"

// consts:
#define MAP_SIZE 8
#define MAX_NAME_SIZE 20

#define X_CHAR 'X'
#define O_CHAR 'O'
#define EMPTY 0
#define P1_VALUE 1 // Black
#define P2_VALUE 2 // White
#define DRAW_SITUATION 3 // Draw 
#define NOT_YET 0 // for the winner

#define MAX_FILE_SIZE 200// 50 + 72 + if needed
#define MAX_FILENAME_SIZE 50 // 2(cm/fm) + 1(-) + 20(name1) + 1(-) + 20(name2) + 1(-) + 2(NO 1-99) + if needed
#define SCOREBOARD_PATH "saved_files/scoreboard.txt"
#define SCOREBOARD_FILENAME "scoreboard.txt"
#define MAX_NUMBER_OF_PLAYERS 30
#define MAX_NUMBER_OF_SAVED_FILES 100

#define VSCODE_TERMINAL 0
#define CMD_TERMINAL 1

#define STARTING_TIME 600 // seconds, later: ask it like screen_type
#define UNDO_TIME_PENALTY 30 // seconds, later: change it later


// structs:
typedef struct
{
    int x;
    int y;
    int value; // 0: empty | 1: Black (X) | 2: White (O)
} disc;

typedef struct
{
    char name[MAX_NAME_SIZE];
    int score;
    int value; // 1: Black (X) | 2: White (O)
    char sign;
    int remaining_time; // sec
    // for undo
    int undoable; // weather can the player use undo move? (preventing double use right after an undo move or at first)
    int undo_moves_left;
    int last_time_consumed; // the time passed for the last move
    int last_score_gained; // the score gained in the last move
} player;


// global varibales:
int map[MAP_SIZE][MAP_SIZE]; // only the values of positions are saved in this array
player p1;
player p2;
int map_copy_p1[MAP_SIZE][MAP_SIZE]; // copy of map, before the last move of p1
int map_copy_p2[MAP_SIZE][MAP_SIZE]; // copy of map, before the last move of p2

int turn = 1; // 1 (p1) or 2 (p2)
int done = 0; // flag
int time_done = 0; // flag, only 1 if the game is over due to time expiration.
int winner = 0; // 0 (not yet) or 1 (p1) or 2 (p2) or 3 (draw!)
int nth_match = 0; // i if existed before, 0 if a new match (valid numbers starting from 1)
int mid_game_save = 0; // flag: 1, only if the game is not over but both playeres agreed to quit and save.
int time_game = 0; // boolean, weather the game has timer (is it in timer mode)
int screen_type = VSCODE_TERMINAL; // check clreaing_screen() function (it is because of the way of clearing screen)
int undo_skip = 0; // 1 only when there was an undo-move, so we must continue; in the while of mainloop 


// the rest is just functions!

// ------------------------------ <base game functions> ------------------------------
int op(int value)
{
    // oposite: B->W / W->B
    int ans;
    switch (value)
    {
    case P1_VALUE:
        ans = P2_VALUE;
        break;
    case P2_VALUE:
        ans = P1_VALUE;
        break;
    case EMPTY:
        perror("Do you really want the op(EMPTY)! it is sus!\n");
        break;
    default:
        perror("The value given to 'op' function is invalid!\n");
        break;
    }
    return ans;
    // return 3 - value;
}

void flip(int* pos_ptr)
{ 
    *pos_ptr = op(*pos_ptr);
    if (*pos_ptr == p1.value)
    {
        p1.score++;
    }
    else if (*pos_ptr == p2.value)
    {
        p2.score++;
    }
    else
    {
        perror("The value of given disc to 'flip' function is invalid!\n");
    }
}

void copy_map(int map_des[MAP_SIZE][MAP_SIZE], int map_src[MAP_SIZE][MAP_SIZE])
{
    for (int i=0; i<MAP_SIZE; i++)
    {
        for (int j=0; j<MAP_SIZE; j++)
        {
            map_des[i][j] = map_src[i][j];
        }
    }
}

int check_placeable(disc d)
{
    // to check the available positions (extra!)
    // 0 if no / 1 if yes
    if (map[d.x][d.y] != EMPTY)
    {
        return 0; // there is already a disc in this position
    }
    int vi, vj;
    for (vi = -1; vi<=1; vi++)
    {
        for (vj = -1; vj<=1; vj++)
        {
            // process the directoin
            int op_discs_count = 0;
            for (int i=d.x+vi, j=d.y+vj; 0<=i && i<MAP_SIZE && 0<=j && j<MAP_SIZE; i+=vi, j+=vj)
            {
                if (map[i][j] == op(d.value))
                {
                    op_discs_count++;
                }
                else if (map[i][j] == d.value)
                {
                    if (op_discs_count)
                    {
                        return 1;
                    }
                    else
                    {
                        // this direction doesn't work
                        break;
                    }
                }
                else
                {
                    // empty => this direction doesn't work
                    break;
                }
            }
        }
    }
    return 0;
}

int check_and_action(disc d)
{
    // check if this is a valid move (return 0 or 1)
    // and if it is valid, this function does the needed actions
    if (map[d.x][d.y] != EMPTY)
    {
        return 0; // there is already a disc in this position
    }
    int vi, vj;
    int found = 0;
    for (vi = -1; vi<=1; vi++)
    {
        for (vj = -1; vj<=1; vj++)
        {
            // process the directoin
            int op_discs_count = 0;
            for (int i=d.x+vi, j=d.y+vj; 0<=i && i<MAP_SIZE && 0<=j && j<MAP_SIZE; i+=vi, j+=vj)
            {
                if (map[i][j] == op(d.value))
                {
                    op_discs_count++;
                }
                else if (map[i][j] == d.value)
                {
                    if (op_discs_count)
                    {
                        // do the action
                        // first put the new disc
                        map[d.x][d.y] = d.value;
                        for (int k=1; k<=op_discs_count; k++)
                        {
                            flip(&map[d.x + k*vi][d.y + k*(vj)]);
                        }
                        found = 1;
                        break; // the process of this direction is done successfully
                    }
                    else
                    {
                        // this direction doesn't work
                        break;
                    }
                }
                else
                {
                    // empty => this direction doesn't work
                    break;
                }
            }
        }
    }
    return found;
}

int check_any_move()
{
    disc d;
    d.value = turn;
    for (int i=0; i<MAP_SIZE; i++)
    {
        for (int j=0; j<MAP_SIZE; j++)
        {
            d.x = i, d.y = j;
            if (check_placeable(d))
            {
                return 1;
            }
        }
    }
    return 0;
}
// ------------------------------ </base game functions> ------------------------------


// ------------------------------ <screen> ------------------------------
void print_map()
{
    // check Note1
    printf("  A B C D E F G H\n");
    for (int j=0; j<MAP_SIZE; j++)
    {
        printf("%d", j+1);
        for (int i=0; i<MAP_SIZE; i++)
        {
            switch (map[i][j])
            {
            case EMPTY:
                disc d;
                d.value = turn;
                d.x = i, d.y = j;
                if (check_placeable(d))
                    printf(" *");
                else
                    printf(" -");
                break;
            case P1_VALUE:
                printf(" %c", p1.sign);
                break;
            case P2_VALUE:
                printf(" %c", p2.sign);
                break;           
            default:
                printf("pos(x=%d, y=%d) value is %d which is invalid\n", i, j, map[i][j]);
                perror("So there is a problem (check the last line of output)\n");
                break;
            }
        }
        printf("\n");
    }
}

void clear_screen()
{
    switch (screen_type)
    {
    case VSCODE_TERMINAL:
        printf("\e[1;1H\e[2J");
        break;
    case CMD_TERMINAL:
        #ifdef _WIN32 
            system("cls");   
        #elif __APPLE__
            system(“clear”);
        #elif __linux__ 
            system(“clear”);
        #endif
        break;
    default:
        printf("-------------------- You didn't choose a methode for clearing screen --------------------\n");
        printf("------------------------------------ The next frame -------------------------------------\n");
        break;
    }
}

void update_screen()
{
    clear_screen();
    print_map();
    printf("\n");
    if (time_game)
    {
        printf("player 1: %20s (%c) \t score: %2d \t time: %2d:%2d \t undo moves left: %d\n", p1.name, p1.sign, p1.score,
        p1.remaining_time/60, p1.remaining_time%60, p1.undo_moves_left);
        printf("player 2: %20s (%c) \t score: %2d \t time: %2d:%2d \t undo moves left: %d\n", p2.name, p2.sign, p2.score,
        p2.remaining_time/60, p2.remaining_time%60, p2.undo_moves_left);
    }
    else
    {
        printf("player 1: %20s (%c) \t score: %2d\n", p1.name, p1.sign, p1.score);
        printf("player 2: %20s (%c) \t score: %2d\n", p2.name, p2.sign, p2.score);
    }
}
// ------------------------------ </screen> ------------------------------


// ------------------------------ <scoreboard> ------------------------------
int read_scoreboard(player arr_players[MAX_NUMBER_OF_PLAYERS])
{
    // returns the number of players found in the file
    FILE *fptr = fopen(SCOREBOARD_PATH, "r");
    if (fptr == NULL)
    {
        fclose(fptr);
        // creating scoreboard for the first time
        fptr = fopen(SCOREBOARD_PATH, "w");
        fclose(fptr);
        return 0;
    }
    int i=0;
    int output = fscanf(fptr, "%s %d", arr_players[i].name, &(arr_players[i].score));
    // output = EOF or 0 if there is no match in fscanf
    while (output != EOF && output != 0)
    {
        fgetc(fptr); // the \n
        i++;
        output = fscanf(fptr, "%s %d", arr_players[i].name, &(arr_players[i].score));
    }
    fclose(fptr);
    return i;
}

void write_scoreboard(player arr_players[], int len)
{
    // overwrite on the scoreborad file
    FILE *fptr = fopen(SCOREBOARD_PATH, "w");
    for (int i=0; i<len; i++)
    {
        fprintf(fptr, "%s %d\n", arr_players[i].name, arr_players[i].score);
    }
    fclose(fptr);
}

int update_player_score_add_it_if_not_existed(player arr_players[], int len, player p)
{
    int found = 0;
    for (int i=0; i<len; i++)
    {
        if (!strcmp(arr_players[i].name, p.name))
        {
            arr_players[i].score += p.score;
            found = 1;
        }
    }
    if (!found)
    {
        arr_players[len] = p;
        len++;
    }
    return len;
}

void update_scoreboard()
{
    // read
    player arr_players[MAX_NUMBER_OF_PLAYERS];
    decode_manager(SCOREBOARD_FILENAME);
    int len = read_scoreboard(arr_players);
    // check and update
    len = update_player_score_add_it_if_not_existed(arr_players, len, p1);
    len = update_player_score_add_it_if_not_existed(arr_players, len, p2);
    // rewrite
    write_scoreboard(arr_players, len);
    encode_manager(SCOREBOARD_FILENAME);
    // free(arr_players);
}


void bubble_sort(player arr_players[], int size)
{
    // Descending
    int i, j;
    for (i=0; i<size; i++)
    {
        for (j=0; j<size-i-1; j++)
        {
            if (arr_players[j].score < arr_players[j+1].score)
            {
                player temp = arr_players[j];
                arr_players[j] = arr_players[j+1];
                arr_players[j+1] = temp;
            }
        }
    }
}

void show_scoreboard()
{
    player arr_players[MAX_NUMBER_OF_PLAYERS];
    decode_manager(SCOREBOARD_FILENAME);
    int len = read_scoreboard(arr_players);
    bubble_sort(arr_players, len);
    printf("==================== SCOREBOARD ====================\n");
    for (int i=0; i<len; i++)
    {
        printf("Rank: %2d | name: %19s | score: %3d\n", i+1, arr_players[i].name, arr_players[i].score);
    }
    printf("====================================================\n");
    encode_manager(SCOREBOARD_FILENAME);
}
// ------------------------------ </scoreboard> ------------------------------



// ------------------------------ <data> ------------------------------
void process_data_and_write_on_file(const char* filename)
{
    char path[MAX_FILENAME_SIZE+15];
    strcpy(path, "./saved_files/");
    strcat(path, filename);
    FILE *fptr = fopen(path, "w");
    fprintf(fptr, "%d\n", time_game);// boolean, check where I defined time_game
    if (time_game)
    {
        fprintf(fptr, "%s %d %d %d\n", p1.name, p1.score, p1.remaining_time, p1.undo_moves_left);// player1 data
        fprintf(fptr, "%s %d %d %d\n", p2.name, p2.score, p2.remaining_time, p2.undo_moves_left);// player2 data
    }
    else
    {
        fprintf(fptr, "%s %d\n", p1.name, p1.score);// player1 data
        fprintf(fptr, "%s %d\n", p2.name, p2.score);// player2 data
    }
    fprintf(fptr, "%d\n", winner); // check where I defined winner
    fprintf(fptr, "%d\n", turn);
    // map -> check Note1
    for (int j=0; j<MAP_SIZE; j++)
    {
        for (int i=0; i<MAP_SIZE; i++)
        {
            fprintf(fptr, "%d ", map[i][j]);
        }
        fprintf(fptr, "\n");
    }
    fclose(fptr);
}

void read_from_file_and_process_data(const char* filename)
{
    char path[MAX_FILENAME_SIZE+15];
    strcpy(path, "./saved_files/");
    strcat(path, filename);
    FILE *fptr = fopen(path, "r");
    fscanf(fptr, "%d", &time_game);
    // no '\n' because '\n' is considered "white space"
    if (time_game)
    {
        fscanf(fptr, "%s %d %d %d", &p1.name, &p1.score, &p1.remaining_time, &p1.undo_moves_left);
        fscanf(fptr, "%s %d %d %d", &p2.name, &p2.score, &p2.remaining_time, &p2.undo_moves_left);
    }
    else
    {
        fscanf(fptr, "%s %d", &p1.name, &p1.score);
        fscanf(fptr, "%s %d", &p2.name, &p2.score);
    }
    p1.sign = X_CHAR, p2.sign = O_CHAR; // just to make sure
    p1.value = P1_VALUE, p2.value = P2_VALUE; // just to make sure
    p1.undoable = 0; p2.undoable = 0; 
    // todo: read the remaining time and available undos for each player
    fscanf(fptr, "%d", &winner);
    fscanf(fptr, "%d", &turn);
    done = (winner != 0); // winner is 0 only when the game is not finished (done = 0)
    // map -> check Note1
    for (int j=0; j<MAP_SIZE; j++)
    {
        for (int i=0; i<MAP_SIZE; i++)
        {
            fscanf(fptr, "%d", &map[i][j]);
        }
    }
    fclose(fptr);
}


int files_in_dir(char filenames[][MAX_FILENAME_SIZE])
{
    DIR *d;
    struct dirent *dir;
    d = opendir("./saved_files"); // I know this is bad, vali khob ...
    if (d)
    {
        int i=0;
        while ((dir = readdir(d)) != NULL)
        {
            strcpy(filenames[i], dir->d_name);
            i++;
        }
        closedir(d);
        return i;
    }
    else
    {
        perror("There is a problem in opening saved_files directory!\n");
        return 0; // :)
    }
}

int build_filename(char filename[MAX_FILENAME_SIZE], int finihsed, char name1[MAX_NAME_SIZE], char name2[MAX_NAME_SIZE], int num)
{
    // this functions sort the names by itself
    // finihsed is a bool
    // num starting from 1 => (num >= 1)
    // returns the lentgh of filename (but I didn't use it at all!)
    char c_or_f; // continuing or finished (match)
    if (finihsed)
        c_or_f = 'f';
    else
        c_or_f = 'c';
    int len;
    // sort the names (name-greater_name)
    if (strcmp(name1, name2) < 0)
    {
        len = sprintf(filename, "%cm %s %s %d.txt", c_or_f, name1, name2, num);
    }
    else
    {
        len = sprintf(filename, "%cm %s %s %d.txt", c_or_f, name2, name1, num);
    }
    return len;
}

void find_filename(char filename[MAX_FILENAME_SIZE], char name1[MAX_NAME_SIZE], char name2[MAX_NAME_SIZE], int finihsed)
{
    // if the file (match) is reopened and existed before:
    if (nth_match > 0)
    {
        if (finihsed)
        {
            char past_filename[MAX_FILENAME_SIZE];
            build_filename(past_filename, !finihsed, name1, name2, nth_match);
            char path[MAX_FILENAME_SIZE+15];
            strcpy(path, "./saved_files/");
            strcat(path, past_filename);
            remove(path);
        }
        build_filename(filename, finihsed, name1, name2, nth_match);
        return;
    }
    // else: a new match (first time saving and creating the file)
    char filenames[MAX_NUMBER_OF_SAVED_FILES][MAX_FILENAME_SIZE];
    int files_num = files_in_dir(filenames);
    char c_or_f, name1_file[MAX_FILENAME_SIZE], name2_file[MAX_FILENAME_SIZE];
    int num; // in case of a conflict! (I don't have time to add these extra features! but I wished to...)
    int count=0; // 1, 2, 3, ...
    for (int i=0; i<files_num; i++)
    {
        sscanf(filenames[i], "%cm %s %s %d.txt", &c_or_f, name1_file, name2_file, &num);
        int if_n1n2 = (!strcmp(name1, name1_file)) && (!strcmp(name2, name2_file));
        int if_n2n1 = (!strcmp(name2, name1_file)) && (!strcmp(name1, name2_file));
        if (if_n1n2 || if_n2n1)
        {
            count++;
        }
        strcpy(name1_file, "");
        strcpy(name2_file, "");
    }
    build_filename(filename, finihsed, name1, name2, count+1);
    // the "filename" is ready :)
}


void save_manager()
{
    // using global variables
    char filename[MAX_FILENAME_SIZE];
    find_filename(filename, p1.name, p2.name, done);
    process_data_and_write_on_file(filename);
    encode_manager(filename);

    if (done)
    {
        update_scoreboard();
    }
    printf("Match has been saved successfully :)\n");
}

void read_manager(char filename[])
{
    // using global variables
    int num; char c, s1[MAX_NAME_SIZE], s2[MAX_NAME_SIZE];
    sscanf(filename, "%cm %s %s %d", &c, s1, s2, &num);
    printf("Start reading match NO. %d\n", num);
    // setting global variables on default values
    done = 0;
    time_done = 0;
    winner = 0;
    turn = 1;
    mid_game_save = 0;
    time_game = 0;
    // except this one :)
    nth_match = num;
    decode_manager(filename);
    read_from_file_and_process_data(filename);
}

// ------------------------------ </data> ------------------------------



// ------------------------------ <menu> ------------------------------
int show_previous_matches(char matchs_filenames[MAX_NUMBER_OF_SAVED_FILES][MAX_FILENAME_SIZE], char name1[MAX_NAME_SIZE], char name2[MAX_NAME_SIZE])
{
    // this function should be used in menu (alternative_main)
    char filenames[MAX_NUMBER_OF_SAVED_FILES][MAX_FILENAME_SIZE];
    int files_num = files_in_dir(filenames);
    char c_or_f, name1_file[MAX_FILENAME_SIZE], name2_file[MAX_FILENAME_SIZE];
    int num, len=0;
    printf("========== Previous games between %s & %s ==========\n", name1, name2);
    for (int i=0; i<files_num; i++)
    {
        sscanf(filenames[i], "%cm %s %s %d.txt", &c_or_f, name1_file, name2_file, &num);
        int if_n1n2 = (!strcmp(name1, name1_file)) && (!strcmp(name2, name2_file));
        int if_n2n1 = (!strcmp(name2, name1_file)) && (!strcmp(name1, name2_file));
        if (if_n1n2 || if_n2n1)
        {
            // add it to the list
            strcpy(matchs_filenames[len], filenames[i]);
            len++;
            // show (printf)
            printf("%d: %20s Vs %20s | match NO. %2d | ", len, name1_file, name2_file, num);
            switch (c_or_f)
            {
            case 'f':
                printf("finished match\n");
                break;
            case 'c':
                printf("continuing match\n");
                break;
            default:
                perror("SOMEBODY CHANGED THE DOCUMENTS!\n");
                break;
            }
        }
        // change these variables to be empty if the next sscanf(filename) didn't matched
        // so they will be empty and not matchable with name1 and name2
        strcpy(name1_file, "");
        strcpy(name2_file, "");
    }
    if (len == 0)
    {
        printf("No match has been found! Starting a new one is more fun anyway!\n");
    }
    printf("==================== End of the list ====================\n");
    return len;
}


void clear_input()
{
    while (getchar() != '\n')
        ;
}

int ask_a_player_to_quit(player p)
{
    int ready = 0;
    printf("Please answer with only one character per question!\n");
    // I don't have time to check if the user is smart enough to understand the note above!!! 
    printf("Player1 with the name of %s, do you agree to quit and save the match? (y/n): ", p.name);
    int smart = 0;
    while (!smart)
    {
        switch (getchar())
        {
        case '1':
        case 'Y':
        case 'y':
            ready = 1;
            smart = 1;
            break;
        case '0':
        case 'N':
        case 'n':
            ready = 0;
            smart = 1;
            break;
        
        default:
            printf("OPS, try agian! options for a positive answer: y/Y/1 and for a negetive answer: n/N/0\n");
            break;
        }
        clear_input();
        //getchar(); // \n, if the user puts two character in the line, it's his/her fault, I even warned him/her! #jijo :)
    }
    return ready;
}

void quit_protocol()
{
    // eddited: no more return, mid_game_save flag says everything!
    // only for mid-game save and quit
    int ready_p1 = ask_a_player_to_quit(p1);
    if (ready_p1)
    {
        printf("OK, so player1 (%s) agreed to quit\n", p1.name);
        int ready_p2 = ask_a_player_to_quit(p2);
        if (ready_p2)
        {
            printf("OK, so player2 (%s) agreed to quit as well\n", p2.name);
            save_manager();
            mid_game_save = 1;
        }
        else
        {
            printf("OPS! player2: %s doesn't want to quit, so the game continues!\n", p2.name);
        }
    }
    else
    {
        printf("OPS! player1: %s doesn't want to quit, so the game continues!\n", p1.name);
    }
    
}
// ------------------------------ </menu> ------------------------------



// ------------------------------ <undo> ------------------------------
void first_undo(player p)
{
    switch (p.value)
    {
    case P1_VALUE:
        p1.undo_moves_left--;
        copy_map(map, map_copy_p1);
        p1.remaining_time -= UNDO_TIME_PENALTY;
        p1.score -= p1.last_score_gained;
        if (turn == P1_VALUE)
            p2.score -= p2.last_score_gained;
        break;
    
    case P2_VALUE:
        p2.undo_moves_left--;
        copy_map(map, map_copy_p2);
        p2.remaining_time -= UNDO_TIME_PENALTY;
        p2.score -= p2.last_score_gained;
        if (turn == P2_VALUE)
            p1.score -= p1.last_score_gained;
        break;
    
    default:
        perror("turn value is invalid (found in undo_move function)\n");
        break;
    }
}

void second_undo(player p)
{
    switch (p.value)
    {
    case P1_VALUE:
        p1.undo_moves_left--;
        copy_map(map, map_copy_p1);
        p1.remaining_time -= 2 * UNDO_TIME_PENALTY;
        p1.score -= p1.last_score_gained;
        if (turn == P1_VALUE)
        {
            p2.score -= p2.last_score_gained;
            p2.remaining_time += (p1.last_time_consumed + p2.last_time_consumed);
        }
        else
        {
            // requested in enemy turn
            p2.remaining_time += p1.last_time_consumed;
        }
        break;
    
    case P2_VALUE:
        p2.undo_moves_left--;
        copy_map(map, map_copy_p2);
        p2.remaining_time -= 2 * UNDO_TIME_PENALTY;
        p2.score -= p2.last_score_gained;
        if (turn == P2_VALUE)
        {
            p1.score -= p1.last_score_gained;
            p1.remaining_time += (p2.last_time_consumed + p1.last_time_consumed);
        }
        else
        {
            // requested in enemy turn
            p1.remaining_time += p2.last_time_consumed;
        }
        break;
    
    default:
        perror("turn value is invalid (found in undo_move function)\n");
        break;
    }
}

int undo_move(player p)
{
    // returns 1, if successful, 0, otherwise
    if (!p.undoable || p.undo_moves_left <= 0)
        return 0;
    if (p.undo_moves_left == 2)
    {
        first_undo(p);
    }
    else if (p.undo_moves_left == 1)
    {
        second_undo(p);
    }
    // after undo, it is the turn of the one who requested it
    turn = p.value;
    // none of players can use an undo-move right after another (until each of them do a new move individually)
    p1.undoable = 0;
    p2.undoable = 0;
    return 1;
}
// ------------------------------ </undo> ------------------------------



// ------------------------------ <game manager> ------------------------------
int get_input(int arr_ij[2], char sign, int passed_time[1])
{
    // get input the position or the commands (quit, refresh(todo))
    // return -1 if the input is a position
    // 0: if the input is refresh or quit (doesn't matter that they agreed or not)
    // return 1 or 2: when the input is "undo p1" or "undo p2"
    char inp[8];
    time_t start_time, end_time;
    start_time = time(NULL);
    printf("Enter the position you want to put your disc (%c): ", sign);
    scanf("%7s", inp);
    clear_input();
    end_time = time(NULL);
    passed_time[0] = (int) (end_time - start_time);
    if (!strcmp(inp, "quit"))
    {
        printf("So there is a quit request:\n");
        quit_protocol();
        return 0;
    }
    if (!strcmp(inp, "refresh"))
    {
        return 0;
    }
    if (!strncmp(inp, "undo-p", 5))
    {
        // untodo
        // "undo p1"  or  "undo p2"
        arr_ij[0] = -1; arr_ij[1] = -1; // to make it say the warning message in process_turn
        return (int) (inp[6] - '0'); // producing junk input_type in process_turn
    }
    arr_ij[0] = (int) (inp[0] - 'A');
    arr_ij[1] = (int) (inp[1] - '1');
    return -1;
}


void subtract_time(player p, int passed)
{
    if (!time_game)
    {
        // then this should not work
        return;
    }
    // todo last time added must be saved
    switch (p.value)
    {
    case P1_VALUE:
        p1.remaining_time -= passed;
        if (p1.remaining_time <= 0)
        {
            done = 1;
            time_done = 1;
            p1.remaining_time = 0;
            return;
        }
        break;
    
    case P2_VALUE:
        p2.remaining_time -= passed;
        if (p2.remaining_time <= 0)
        {
            done = 1;
            time_done = 1;
            p2.remaining_time = 0;
            return;
        }
        break;

    default:
        perror("Invalid player given to the process_turn and subtract\n"); // impossible
        break;
    }
}


void process_turn(player p)
{
    int accepted = 0;
    int delta_time = 0;
    int arr_ij[2], passed_time[1]; // I know the refrenceByPointer exist but if you are me you know why!
    int input_type;
    int temp_map[MAP_SIZE][MAP_SIZE];
    while (!accepted)
    {
        // wtfbug!
        passed_time[1] = 0;
    input_agian:
        input_type = get_input(arr_ij, p.sign, passed_time); // position -1/junk, (refresh and quit) 0, undo 1/2
        if (mid_game_save)
        {
            // attemp to quit -> successful
            return;
        }
        if (input_type == 1 || input_type == 2)
        {
            if (!time_game)
            {
                printf("You are playing a No-Timer game, you cannot undo!\n");
                goto input_agian;
            }
            // untodo
            player pdo;
            if (input_type == 1)
                pdo = p1;
            else
                pdo = p2;
            if (undo_move(pdo))
            {
                subtract_time(pdo, 0); // just to check if the time is over for him or not
                if (time_game && time_done)
                    return;
                // this werird code in here with two return, has a personal reason, for understanding the idea behind it!
                undo_skip = 1;
                return;
            }
            else
            {
                delta_time += passed_time[0];
                subtract_time(p, passed_time[0]);
                if (time_game && time_done)
                    return;
                update_screen();
                printf("You cannot use undo in this turn!\n");
                goto input_agian;
            }
        }
        if (input_type == 0)
        {
            // refresh (or an unsuccessful quit)
            delta_time += passed_time[0];
            subtract_time(p, passed_time[0]);
            if (time_game && time_done)
                return;
            update_screen();
            goto input_agian;
        }
        else
        {
            // (input_type == -1) or wrong undo syntax
            delta_time += passed_time[0];
            subtract_time(p, passed_time[0]);
            if (time_game && time_done)
                return;
            if (!(0 <= arr_ij[0] && arr_ij[0] < MAP_SIZE && 0 <= arr_ij[1] && arr_ij[1] < MAP_SIZE))
            {
                printf("Wrong syntax, you must give the position like this: F6\n");
                printf("Also for using available commands (quit, refresh) just write them.\n");
                printf("And for using undo, use \"undo-p1\" or \"undo-p2\" according to who wants to undo\n");
                goto input_agian;
            }
        }
        // ---------------------------------------------------------------- correct input (not correct disk)
        copy_map(temp_map, map);
        int start_score = p.score, end_score;
        disc d;
        d.value = p.value;
        d.x = arr_ij[0], d.y = arr_ij[1];
        if (check_and_action(d))
        {
            accepted = 1;
            // undo prepration (log of last move)
            switch (p.value)
            {
            case P1_VALUE:
                end_score = p1.score;
                p1.last_score_gained = end_score - start_score;
                p1.last_time_consumed = delta_time;
                copy_map(map_copy_p1, temp_map);
                p1.undoable = 1;
                break;
            case P2_VALUE:
                end_score = p2.score;
                p2.last_score_gained = end_score - start_score;
                p2.last_time_consumed = delta_time;
                copy_map(map_copy_p2, temp_map);
                p2.undoable = 1;
                break;
            default:
                perror("Value of p.value in process_turn function is invalid (not 1 or 2)\n"); // impossible!
                break;
            }
            printf("%s successfully puts a %c at %c%d\n", p.name, p.sign, arr_ij[0]+'A', arr_ij[1]+1);
        }
        else
        {
            printf("You can't put %c at %c%c\n", p.sign, arr_ij[0]+'A', arr_ij[1]+'1');
        }
    }
}


void find_and_update_the_winner()
{
    // only called at end_of_game function
    
    // first check for time expiration
    if (time_game && time_done)
    {
        if (p1.remaining_time <= 0 && p2.remaining_time > 0)
        {
            winner = P2_VALUE;
            p1.score = 0;
        }
        else if (p2.remaining_time <= 0 && p1.remaining_time > 0)
        {
            winner = P1_VALUE;
            p2.score = 0;
        }
        else
        {
            perror("p1 and p2 both ran out of time! Impossible!!!\n");
            printf("p1 time: %d p2 time: %d\n", p1.remaining_time, p2.remaining_time);
            winner = DRAW_SITUATION; // just to do something in case of not finding this bug
        }
        return;    
    }
    // else:
    // now that there is no time expiration,
    // it counts the discs of each player to determine the winner
    int count_p1=0, count_p2=0;
    for (int i=0; i<MAP_SIZE; i++)
    {
        for (int j=0; j<MAP_SIZE; j++)
        {
            switch (map[i][j])
            {
            case P1_VALUE:
                count_p1++;
                break;
            
            case P2_VALUE:
                count_p2++;
                break;

            default:
                // do nothing :)
                break;
            }
        }
    }
    if (count_p1 > count_p2)
        winner = P1_VALUE;
    else if (count_p2 > count_p1)
        winner = P2_VALUE;
    else // count_p1 == count_p2
        winner = DRAW_SITUATION;
        // bug-design
}


void init(char username1[MAX_NAME_SIZE], char username2[MAX_NAME_SIZE], int timer)
{
    // this function is called only at the start of a match
    strcpy(p1.name, username1);
    p1.score = 0;
    p1.value = P1_VALUE;
    p1.sign = X_CHAR;
    p1.remaining_time = STARTING_TIME;
    p1.undoable = 0;
    p1.undo_moves_left = 2;

    strcpy(p2.name, username2);
    p2.score = 0;
    p2.value = P2_VALUE;
    p2.sign = O_CHAR;
    p2.remaining_time = STARTING_TIME;
    p2.undoable = 0;
    p2.undo_moves_left = 2;

    // clear the map
    for (int i=0; i<MAP_SIZE; i++)
    {
        for (int j=0; j<MAP_SIZE; j++)
        {
            map[i][j] = EMPTY;
        }
    }
    // put the 4 primary discs
    map[MAP_SIZE/2 -1][MAP_SIZE/2 -1] = map[MAP_SIZE/2][MAP_SIZE/2] = P2_VALUE; // O (White)
    map[MAP_SIZE/2][MAP_SIZE/2 -1] = map[MAP_SIZE/2 -1][MAP_SIZE/2] = P1_VALUE; // X (Black)
    
    //   A B C D E F G H
    // 1 - - - - - - - -
    // 2 - - - - - - - -
    // 3 - - - - - - - -
    // 4 - - - O X - - -
    // 5 - - - X O - - -
    // 6 - - - - - - - -
    // 7 - - - - - - - -
    // 8 - - - - - - - -

    turn = 1;
    done = 0;
    time_done = 0;
    winner = 0;
    nth_match = 0;
    mid_game_save = 0;
    time_game = timer;
}


void end_of_game()
{
    // for update_screen turn doesn't matter because there will be no * in the map. (except for time_done == 1)
    // the game is over!
    // this function is only called when done == 1
    update_screen();
    find_and_update_the_winner();
    switch (winner)
    {
    case NOT_YET:
        printf("So we have a continuing match that players agreed to quit!\n"); // Impossible
        break;
    case P1_VALUE:
        printf("%20s is the winner\n", p1.name);
        break;
    case P2_VALUE:
        printf("%20s is the winner\n", p2.name);
        break;
    case DRAW_SITUATION:
        printf("WOW, it is a perfect draw! Congratulation to both players\n");
        break;
    default:
        perror("There is problem with turn value (invalid)\n");
        break;
    }
    save_manager();
}


void mainloop()
{
    if (done)
    {
        // a finished game is opened
        update_screen();
        printf("This match has been finished before!\n");
        return;
    }
    while (!done)
    {
        update_screen();
        switch (turn)
        {
        case P1_VALUE:
            process_turn(p1);
            break;
        case P2_VALUE:
            process_turn(p2);
            break;
        default:
            perror("Turn is out of valid range!\n");
            break;
        }
        if (mid_game_save || (time_game && time_done))
        {
            break;
        }
        if (undo_skip)
        {
            undo_skip = 0;
            continue;
        }
        turn = 3 - turn;// 1 -> 2, 2 -> 1
        if (!check_any_move())
        {
            
            printf("No legal move available for player%d, so his/her turn is skipped!\n", turn);
            turn = 3 - turn;
            if (!check_any_move())
            {
                printf("No legal move available for any player, the game is over!\n");
                done = 1;
            }
        }
    }
    if (mid_game_save)
    {
        printf("Thank you for playing the game!\n");
    }
    else
    {
        end_of_game();
    }
}

// ------------------------------ </game manager> ------------------------------



// ------------------------------ <starter> ------------------------------

// also show_scoreboard but it is in <scoardboard> section for obvious reasons

void play()
{
    char name1[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
    printf("Consider the lentgh of a player name cannot exceed the limit of %d characters\n", MAX_NAME_SIZE-1);
    printf("Please enter player1 name: ");
    scanf("%s", name1);
    clear_input();
    printf("Now enter player2 name: ");
    scanf("%s", name2);
    clear_input();
    printf("Please choose the number according to the options:\n");
    printf("If you want a new match put 0, otherwise put the number of a match you've played before\n");
    printf("0: New Match\n");
    char matchs_filenames[MAX_NUMBER_OF_SAVED_FILES][MAX_FILENAME_SIZE];
    int len = show_previous_matches(matchs_filenames, name1, name2);
    int cmd;
    scanf("%d", &cmd);
    clear_input();
    while (cmd > len || cmd < 0)
    {
        printf("Your number is out of range of [%d, %d]\n", 0, len);
        scanf("%d", &cmd);
        clear_input();
    }
    if (cmd == 0)
    {
        int timer; // temp (should be boolean)
    timer_again:
        printf("Please choose the number according to the options:\n");
        printf("Do you want timer in your game?\n");
        printf("Timer Mode: \t 0 -> OFF \t 1 -> ON\n");
        scanf("%d", &timer);
        clear_input();
        if (timer != 0 && timer != 1)
        {
            printf("Please choose between 0 and 1, this is so simple!\n");
            goto timer_again;
        }
        init(name1, name2, timer);
        mainloop();
    }
    else
    {
        read_manager(matchs_filenames[cmd-1]);
        mainloop();
    }
}


// an alternative for main!
void alternative_main()
{
    printf("Hello Othello!\n");
    // show scoreboard or play
    int option;
option_agian:
    printf("Please choose the number according to the options:\n");
    printf("0 -> play \t\t 1 -> scoreboard\n");
    scanf("%d", &option);
    switch (option)
    {
    case 0:
        play();
        break;
    
    case 1:
        show_scoreboard();
        break;

    default:
        printf("Please choose between 0 and 1, this is so simple!\n");
        goto option_agian;
    }

}

// ------------------------------ </starter> ------------------------------



int main()
{
    hard_code();
    printf("Which terminal are you using? vscode debuger (put 0) or cmd and others (put 1): ");
    scanf("%d", &screen_type);
    clear_input();
    // main menu
    int another_one = 1;
    while (another_one)
    {
        alternative_main();
        printf("The game is over, press ENTER to go back to main menu\n");
        clear_input(); // check here
        clear_input();
        clear_screen();
        char yn;
        printf("Main Menu:\n");
    ask_again:
        printf("Do you want to continue using the program? (y/n): ");
        yn = getchar();
        clear_input();
        switch (yn)
        {
        case '1':
        case 'Y':
        case 'y':
            another_one = 1;
            break;
        
        case '0':
        case 'N':
        case 'n':
            another_one = 0;
            break;
        
        default:
            printf("Please answer with only one character per question!\n");
            // I don't have time to check if the user is smart enough to understand the note above!!! 
            printf("OPS, try agian! options for a positive answer: y/Y/1 and for a negetive answer: n/N/0\n");
            goto ask_again;
        }
    }
    printf("The program is closed.\n");
}
