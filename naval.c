/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>				/* strcmp() */
#include <limits.h>				/* INT_MAX */

/* Magic Numbers */
#define MAX_SHIPS			15	/* Max number of ships */
#define BUFF_LEN			22	/* Max characters per line */
#define ERR_PARAMS_MISSING	10	/* Not enough parameters */
#define ERR_RULES_MISSING	20	/* Rules file missing or unreadable */
#define ERR_MAPS_MISSING	30	/* Map file missing or unreadable */
#define ERR_RULES_INVALID	40	/* Error processing rules file */
#define ERR_MAP_OVERLAP		50	/* Ships overlap in map */
#define ERR_MAP_OOB			51	/* Ship goes out of bounds */
#define ERR_MAPS_INVALID	52	/* Other error processing map file */
#define ERR_BAD_GUESS		60	/* Input runs out before game is over */

/* Struct definitions */
typedef struct Ship {
    unsigned int length;
    unsigned int xPos;
    unsigned int yPos;
    char direction;
} Ship;

typedef struct Grid {
    unsigned int width;
    unsigned int height;
} Grid;

/* Function prototypes */
/* Parse Functions */
/* Parses the rules file and stores results into relevant variables */
int parse_rules(FILE *rulesFile, Ship *ships, Grid *board,
        unsigned int *numShips);
/* Parses the map file and stores results into the relevant variables */
int parse_map(FILE *mapFile, Ship *ships, Grid *board,
        unsigned int *numShips);
/* Uses information parsed from rules and map files to populate the
 * solutions array (answer) */
int place_ships(Ship *ships, unsigned int numShips, int **answer);

/* Helper Functions */
/* Check whetehr the line is of the correct size */
int check_length(char *line);
/* Check whetehr the ship with ID n has been sunk */
int is_sunk(int n, Grid *board, int **answer);
/* Check whether the game is over */
int is_game_over(Grid *board, int **answer);
/* Parse a line for two unsigned integers and store the result into
 * the appropriate variables */
int read_two_uints(char *line, unsigned int *a, unsigned int *b);
/* Execute the user's guess */
void make_guess(unsigned int *xGuess, unsigned int *yGuess, Grid *board,
        int **answer);

/* Interaction Functions */
/* Display the current board state to the user */
void display_board(unsigned int gWidth, unsigned int gHeight,
        int **answer);
/* Display the input prompt to the user */
void display_prompt(void);
/* Store the user's input into the appropriate variables */
int get_prompt(unsigned int *xGuess, unsigned int *yGuess);

/* Error Functions */
/* These error functions print an error message to stdout and return a
 * unique error code */
/* Called if there are not enough parameters passed to the program */
int params_missing(void);
/* Called if the rules file is missing or unreadable */
int rules_missing(void);
/* Called if the map file is missing or unreadable */
int maps_missing(void);
/* Called if there is an error processing the rules file */
int rules_invalid(void);
/* Called if ships overlap in the map file */
int map_overlap(void);
/* Called if a ship goes out of bounds */
int map_oob(void);
/* Called if there is a syntax error in the map file */
int maps_invalid(void);
/* Called if input runs out before the game is over */
int bad_guess(void);

/* Functions */
int main(int argc, char *argv[] )
{
    FILE *rules, *map;			/* Pointers to rules/map file streams */
    Grid board;					/* Store info about the game board */
    Ship ships[MAX_SHIPS];		/* Store info about each ship */
    int **answer;				/* Store the solution of the game */
    unsigned int numShips = 0;	/* Store the total numberof ships */
    unsigned int xGuess, yGuess;/* Store the player's current guess */
    
    /* Temp Variables */
    int status = 0;				/* exit status */
    int i = 0, j = 0;			/* loop indeces */

    /* There must be at least three parameters */
    if(argc < 3) {
        return params_missing();
    }

    /* Save rules file into "rules", it's OK if "standard.rules" DNE
     * Rules file must be readable unless it is standard.rules */
    if((rules = fopen(argv[1], "r")) == NULL) { 
        if(!strcmp(argv[1], "standard.rules")) {
            fclose(rules);
            /* Looking for "standard.rules" but file DNE so create it
             * with the default contents */
            rules = fopen(argv[1], "w");
            fprintf(rules, "8 8\n5\n5\n4\n3\n2\n1\n\n");
            fclose(rules);
            rules = fopen(argv[1], "r");
        } else {
            return rules_missing();
        }
        fclose(rules);
    }

    /* Save map file into "map"
     * Map file must be readable */
    if((map = fopen(argv[2], "r")) == NULL ) {
        fclose(map);
        return maps_missing();
    }

    /* Save and check rules file
     * Rules must conform to specification */
    if((status = parse_rules(rules, ships, &board, &numShips))) {
        return status;
    }

    /* Initialize the answer array */
    answer = (int**)malloc(board.width * sizeof(int*));
    for(i = 0; i < board.width; i++) {
        answer[i] = (int*)malloc(board.height * sizeof(int));
        for(j = 0; j < board.height; j++) {
            answer[i][j] = 1;
        }
    }

    /* Save and check map file
     * Map must conform to specification */
    if((status = parse_map(map, ships, &board, &numShips))) {
        return status;
    }

    /* Populate the solution */
    if((status = place_ships(ships, numShips, answer))) {
        return status;
    }

    /* Interaction Loop */
    for(;;) {
        display_board(board.width, board.height, answer);
        display_prompt();
        switch(get_prompt(&xGuess, &yGuess)) {
            case 0:
                make_guess(&xGuess, &yGuess, &board, answer);
                break;
            case 1:
                printf("Bad guess\n");
                break;
            case -1:
                return bad_guess();
        }
    }
    return 0;
}

/* Error functions */
int params_missing(void)
{
    printf("usage: naval rules map\n");
    return ERR_PARAMS_MISSING;
}

int rules_missing(void)
{
    printf("Missing rules file\n");
    return ERR_RULES_MISSING;
}

int maps_missing(void)
{
    printf("Missing map file\n");
    return ERR_MAPS_MISSING;
}

int rules_invalid(void)
{
    printf("Error in rules file\n");
    return ERR_RULES_INVALID;
}

int map_invalid(void)
{
    printf("Error in map file\n");
    return ERR_MAPS_INVALID;
}

int map_oob(void)
{
    printf("Out of bounds in map file\n");
    return ERR_MAP_OOB;
}

int map_overlap(void)
{
    printf("Overlap in map file\n");
    return ERR_MAP_OVERLAP;
}

int bad_guess(void)
{
    printf("Bad guess\n");
    return ERR_BAD_GUESS;
}

/* Parse Functions */
int parse_rules(FILE *rules, Ship *ships, Grid *board,
        unsigned int *numShips)
{
    char line[BUFF_LEN]; /* Line buffers */
    int i; 

    /* First line should be two positive integers */
    fgets(line, BUFF_LEN, rules);
    if(read_two_uints(line, &board->width, &board->height)) {
        return rules_invalid();
    }
    if(board->width <= 0 || board->height <= 0) {
        return rules_invalid();
    }

    /* Second line should be a positive integer representing
     * the total number of ships */
    fgets(line, BUFF_LEN, rules);
    if(!check_length(line)) {
        return rules_invalid();
    }
    if(sscanf(line, "%u", numShips) != 1) {
        return rules_invalid();
    }
    if(*numShips <= 0) {
        return rules_invalid();
    }
    /* Limit size of numShips */
    if(*numShips > MAX_SHIPS) {
        return rules_invalid();
    }
    
    /* All other lines should be a single positive integer */
    /* Also, we expect there to be the same number of ships described
     * here as defined by the second line of the rules file and saved
     * to numShips */ 
    for(i = 0; i < *numShips; i++) {
        /* Check if we reached EOF prematurely */
        if(fgets(line, BUFF_LEN, rules) == NULL) {
            return rules_invalid();
        }
        /* The line should be < BUFF_LEN */
        if(!check_length(line)) {
            return rules_invalid();
        }
        if(sscanf(line, "%u", &ships[i].length) != 1) {
            return rules_invalid();
        }
        if(ships[i].length < 1) {
            return rules_invalid();
        }
    }

    return 0;
}

int parse_map(FILE *rules, Ship *ships, Grid *board, 
        unsigned int *numShips)
{
    char line[BUFF_LEN];	/* Line buffers */
    int i;
    int maths;				/* Temporary integer for performing math */

    /* All lines should be a two positive integers followed by a char */
    /* Also, we expect there to be the same number of ships described
     * here as defined by the second line of the rules file and saved
     * to numShips */ 
    /* After parsing for errors, we then check for overlap and out of bounds
     * on a line by line basis */
    /* Note: We can assume only one error possible per line */
    for(i = 0; i < *numShips; i++) {
        /* Check if we reached EOF prematurely */
        if(fgets(line, BUFF_LEN, rules) == NULL) {
            return map_invalid();
        }
        /* Check the length of the line */
        if(!check_length(line)) {
            return map_invalid();
        }
        if(sscanf(line, "%u %u %c", &ships[i].xPos, &ships[i].yPos, 
                    &ships[i].direction) != 3) {
            return map_invalid();
        }

        /* ship.direction must be 'N', 'S', 'E', or 'W' */
        if(ships[i].direction != 'N' && ships[i].direction != 'S' && 
                ships[i].direction != 'E' && ships[i].direction != 'W') {
            return map_invalid();
        }

        /* Out of Bounds check */
        /* First check anchor point */
        if(ships[i].xPos > board->width ||
                ships[i].xPos > board->height) {
            return map_oob(); 
        }
        /* Next, check no section of ship is out of bounds */
        switch(ships[i].direction) {
            case 'N':
                maths = ships[i].yPos - (ships[i].length - 1);
                if(maths < 0) {
                    return map_oob();
                }
                break;
            case 'S':
                maths = ships[i].yPos + (ships[i].length - 1);
                if(maths >=	board->height) {
                    return map_oob();
                }
                break;
            case 'E':
                maths = ships[i].xPos + (ships[i].length - 1);
                if(maths >=	board->width) {
                    return map_oob();
                }
                break;
            case 'W':
                maths = ships[i].xPos - (ships[i].length - 1);
                if(maths < 0) {
                    return map_oob();
                }
                break;
            default:
                return map_invalid();
        }

    }
    return 0;
}

int place_ships(Ship *ships, unsigned int numShips, int **answer)
{
    int i,j; /* loop indexes */
    int x,y; /* array position offsets */
    for(i = 0; i < numShips; i++)
    {
        x = 0;
        y = 0;

        for(j = 0; j < ships[i].length; j++) {
            /* Note: 1 means no ship is there */
            if(answer[ships[i].xPos + x][ships[i].yPos + y] != 1) {
                return map_overlap();			
            } else {
                /* i + 2 because 1 means no ship, -1 means miss */
                answer[ships[i].xPos + x][ships[i].yPos + y] = i + 2;
            }

            switch(ships[i].direction) {
                case 'N':
                    y -= 1;
                    break;
                case 'S':
                    y += 1;
                    break;
                case 'E':
                    x += 1;
                    break;
                case 'W':
                    x -= 1;
                    break;
                default:
                    return map_invalid();
            }
        }
    }
    return 0;
}

/* Helper Functions */
int check_length(char *line) {
    if(line[strlen(line) - 1] != '\n') {
        if(strlen(line) == 0) {
            return 0;
        }
        return -1;
    }
    return 1;
}

int is_sunk(int n, Grid *board, int **answer)
{
    int i, j;
    for(i = 0; i < (board->width); i++) {
        for(j = 0; j < (board->height); j++) {
            if(answer[i][j] == n) {
                return 0;
            }
        }
    }
    return 1;
}

int is_game_over(Grid *board, int **answer)
{
    int i,j;
    for(i = 0; i < (board->width); i++) {
        for(j = 0; j < (board->height); j++) {
            if(answer[i][j] > 1) {
                return 0;
            }
        }
    }
    return 1;
}

int read_two_uints(char *line, unsigned int *a, unsigned int *b)
{
    if(!check_length(line)) {
        return -1;
    }
    if(sscanf(line, "%u %u", a, b) != 2) {
        return -1;
    }
    
    /* Check for negative numbers */
    if(*a > INT_MAX || *b > INT_MAX) {
        return -1;
    }
    return 0;
}

void make_guess(unsigned int *xGuess, unsigned int *yGuess, Grid *board,
        int **answer)
{
    if(*xGuess >= board->width || *yGuess >= board->height) {
        printf("Bad guess\n");
    } else if(answer[*xGuess][*yGuess] == 1 || 
            answer[*xGuess][*yGuess] == -1) {
        printf("Miss\n");
        answer[*xGuess][*yGuess] = -1;
    } else {
        printf("Hit\n");
        if(answer[*xGuess][*yGuess] > 0) {
            answer[*xGuess][*yGuess] = -answer[*xGuess][*yGuess];
            if(is_sunk(-answer[*xGuess][*yGuess], board, answer)) {
                printf("Ship sunk\n");			
            }
        }
        if(is_game_over(board, answer)) {
            printf("Game over\n");
            exit(0);
        }
    }
}

/* Interaction Functions */
void display_board(unsigned int gWidth, unsigned int gHeight,
        int **answer)
{
    unsigned int i, j;
    for(i = 0; i < gHeight; i++) {
        for(j = 0; j < gWidth; j++) {
            switch(answer[j][i]) {
                case 1: /* No ship here */
                    printf(".");
                    break;
                case -1: /* Missed guess here */
                    printf("/");
                    break;
                default:
                    if(answer[j][i] > 1) {
                        printf(".");
                    } else {
                        printf("*");
                    }
            }
        }
        printf("\n");
    }
}

void display_prompt(void)
{
    printf("(x,y)>");
}

int get_prompt(unsigned int *xGuess, unsigned int *yGuess)
{
    char line[BUFF_LEN]; /* Line buffer */
    char c;				/* Temporary for checking single characters */

    fgets(line, BUFF_LEN, stdin);
    /* NOTE: This will return -1 is EOF */
    if(check_length(line) == 0) {
        return -1;
    /* NOE: This will return 1 if line is > 20 characters */
    } else if(check_length(line) == -1) {
        /* flush the buffer before continuing */
        while((c = getchar()) != '\n') {
            /* If there's no new line, get out now! */
            if(c==EOF) {
                return -1;
            }
        }
        return 1;
    }

    if(read_two_uints(line, xGuess, yGuess)) {
        return 1;
    }
    return 0;
}

