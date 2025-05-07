#include <bits/types/timer_t.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <raylib.h>

#define BUFFER_SIZE 1024 * 1024

#define WIDTH 500
#define HEIGHT 500

/* #define WIDTH 1760 */
/* #define HEIGHT 990 */

typedef enum OP {
    OP_ADD,     // INTERNAL NODE
    OP_MUL,     // INTERNAL NODE
    OP_SIN,     // INTERNAL NODE
    OP_COS,     // INTERNAL NODE
    OP_VAL,     // LEAF     NODE
    OP_X,       // VAR LEAF NODE X 
    OP_Y,       // VAR LEAF NODE Y
} OP;

const char* get_op_name(OP op) {
    switch (op) {
        case OP_ADD: return "ADD";
        case OP_MUL: return "MUL";
        case OP_VAL: return "VAL";
        case OP_X: return "X";
        case OP_Y: return "Y";
        case OP_SIN: return "SIN";
        case OP_COS: return "COS";
        default: return "INVALID";
    }
}

typedef struct ImgGrid {
    Color c;
} ImgGrid;

typedef struct Node {
    struct Node* left;
    struct Node* right;
    char* value;                // LEAF NODE
    OP operation;               // INTERNAL NODE
} Node;

typedef struct Token_Info {
    char** token_list;
    int total_tokens;
} Token_Info;

// CFG Specifically CNF
// S -> aS | aa
typedef struct Production {
    int num_production;         // 2 Productions
    char from_production;       // S
    char** to_production;       // aS and aa
} Production;

typedef Production** Grammar;   // Set of Productions = Grammar

Grammar read_productions(FILE* fp, int n) {

    int pro_count = 0;
    char token[128];
    char buffer[128];
    char to_production_string[512];
    Production** Grammar = (Production**) malloc(sizeof(Production*) * n);

    if(!Grammar) {
        fprintf(stderr, "[-] ERR ALLOCATING GRAMMAR");
    }

    memset(token, 0, 128);
    memset(buffer, 0, 128);
    memset(to_production_string, 0, 512);

    while(fgets(buffer, 128, fp)) {

        Production* p = (Production*) malloc(sizeof(struct Production));

        if(!p) {
            fprintf(stderr, "[-] ERR ALLOCATING PRODUCTION");
        }

        memset(p, 0, sizeof(Production));

        int num_count = 0;

        FILE* buffer_fp = fmemopen(buffer, strlen(buffer), "r");

        char symbol = getc(buffer_fp);
        p->from_production = symbol;

        while(symbol != EOF) {

            symbol = getc(buffer_fp);

            if (symbol == '=' || symbol == '>') {
                continue;
            } else if (symbol == '|') {
                num_count++;
                strcat(to_production_string, token);
                strcat(to_production_string, "|");
                token[0] = '\0';
            } else {
                size_t token_len = strlen(token);
                if (token_len < sizeof(token) - 1) {
                    token[token_len] = symbol;
                    token[token_len + 1] = '\0';
                }            
            }

        }

        size_t token_len = strlen(token);
        token[token_len-2] = '\0';

        strcat(token, &symbol);
        strcat(to_production_string, token);
        token[0] = '\0';

        num_count++;

        char** build_to_production = malloc(sizeof(char*)*num_count);
        if(!build_to_production) {
            fprintf(stderr, "[-] ERR ALLOCATING BUILD_TO_PRODUCTION");
        }
        int num_count_2 = 0;
        char* rule = strtok(to_production_string, "|");

        while (rule != NULL) {
            build_to_production[num_count_2++] = strdup(rule);
            rule = strtok(NULL, "|");
        }

        p->to_production = build_to_production;
        p->num_production = num_count;
        Grammar[pro_count++] = p;

        to_production_string[0] = '\0';
        fclose(buffer_fp);

    };

    return Grammar;

}

void print_grammar(Grammar g, int n) {
    for(int i = 0; i < n; i++) {
        int num = g[i]->num_production;
        for(int j = 0; j < num; j++) {
            printf("%c->%s\n", g[i]->from_production, g[i]->to_production[j]);
        }
    }
}

int find_non_terminal(Grammar g, int n, char c) {
    for(int i = 0; i < n; i++) {
        if (g[i]->from_production == c) {
            return i;
        }
    }
    return -1;
}

char* get_random_production(Grammar g, int from_production) {

    int num_production = g[from_production]->num_production;
    char* to_production = g[from_production]->to_production[rand() % num_production];

    char* production = (char*) malloc(sizeof(char) * (strlen(to_production)));
    production = to_production;

    return production;

}

char* collect_string(char* word, int start, int end) {

    char* dest = (char*) malloc(sizeof(char) * ((end-start) + 1));

    for(int i = 0; i < (end - start); i++) {
        dest[i] = word[start+i];
    }

    dest[end-start] = '\0';

    return dest;

}

char* join(char* left, char* production, char* right) {

    size_t total_size = strlen(left) + strlen(production) + strlen(right) + 1;

    char* dest = (char*) malloc(sizeof(char) * total_size);

    strcpy(dest, left);
    strcat(dest, production);
    strcat(dest, right);

    return dest;

}

void sanitize_string(char* str) {
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (isprint(str[i]) || str[i] == '\n') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

static int word_parsed_count = 0;

void generate_word(Grammar g, int n, char* word) {

    for(int i = 0; i < strlen(word); i++) {

        char* symbol_production;
        char symbol = word[i];
        int is_non_terminal = find_non_terminal(g, n, symbol);

        if(is_non_terminal == -1) {
            continue;
        } else {

            char* left = collect_string(word, 0, i);
            symbol_production = get_random_production(g, is_non_terminal);
            char* right = collect_string(word, i+1, strlen(word));

            word = join(left, symbol_production, right);

            generate_word(g, n, word);
            word_parsed_count++;

            if(word_parsed_count == 1) {

                FILE* fp = fopen("./production_output", "w");

                if(!fp) {
                    fprintf(stderr, "[-] ERROR OPENING THE FILE 'production_output'\n");
                    exit(-1);
                }

                char* production_output_string = collect_string(word, 0, strlen(word));

                sanitize_string(production_output_string);

                if(fwrite(production_output_string , 1, strlen(production_output_string), fp) == 0) {
                    fprintf(stderr, "[-] ERROR WRITING TO THE FILE 'production_output'\n");
                    exit(-1);
                };

                fclose(fp);

            } else {

                return;

            }

        }

    }

    return;

}

Node* get_node() {

    Node* new_node = malloc(sizeof(struct Node));

    new_node->left = NULL;
    new_node->right = NULL;
    new_node->value = NULL;

    return new_node;

}

Token_Info* tokenize_expression(FILE* fp) {

    char token[128];
    char* buffer = malloc(BUFFER_SIZE);

    if (buffer == NULL) {
        fprintf(stderr, "[-] MEMORY ALLOCATION FAILED");
        exit(-1);
    }

    memset(token, 0, 128);
    memset(buffer, 0, BUFFER_SIZE);

    if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
        fprintf(stderr, "[-] ERR READING FROM OUTPUT FILE\n");
        exit(-1);
    };

    FILE* buffer_fp = fmemopen(buffer, strlen(buffer), "r");

    if(buffer_fp == NULL) {
        fprintf(stderr, "[-] ERR READING FROM OUTPUT FILE\n");
        exit(-1);
    };

    if (strlen(buffer) == 1) {
        fprintf(stderr, "[-] PRODUCTION_OUTPUT OR PRODUCTION_OUTPUT2 FILE SEEMS EMPTY\n");
        exit(-1);
    };

    printf("%s\n", buffer);


    //--------------------------------------------------- TOKENIZATION

    int curr_index = 0;
    int total_tokens = 0;
    char symbol = buffer[curr_index];

    char** token_list = (char**) malloc(sizeof(char*) * 1);

    while(buffer[curr_index] != '\0') {

        symbol = buffer[curr_index];

        if (strlen(token) + 1 < sizeof(token)) {

            char temp[2] = {symbol, '\0'};
            strcat(token, temp);

        } else {

            fprintf(stderr, "[-] ERR TOKEN OVERFLOW");
            exit(-1);

        }

        if (strcmp(token, "mul") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "add") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "sin") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "cos") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "(") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, ")") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, ",") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "y") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "x") == 0) {

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(token);
            token[0] = '\0';

        } else if (strcmp(token, "-") == 0 || isdigit(symbol)) {

            int temp = curr_index;
            char sym = buffer[temp];

            if (sym == '-') {
                temp++;
                sym = buffer[temp];
            }

            while(isdigit(sym) || sym == '.') {
                temp++;
                sym = buffer[temp];
            }

            char* num = collect_string(buffer, curr_index, temp);

            token_list = realloc(token_list, (total_tokens + 1) * sizeof(char*));
            token_list[total_tokens++] = strdup(num);
            token[0] = '\0';

            curr_index = temp-1;

        }

        curr_index++;

    }

    Token_Info* token_info = (Token_Info*) malloc(sizeof(Token_Info));
    token_info->token_list = token_list;
    token_info->total_tokens = total_tokens;

    free(buffer);
    buffer = NULL;

    return token_info;

}

Node* parse_tree(Token_Info token_info, int* token_to_parse) {

    if (*token_to_parse >= token_info.total_tokens) {
        return NULL;
    }

    char* token = token_info.token_list[*token_to_parse];

    if (strcmp(token, "mul") == 0 || strcmp(token, "add") == 0) {

        Node* node = get_node();
        node->operation = (strcmp(token, "mul") == 0) ? OP_MUL : OP_ADD;

        (*token_to_parse)++;
        (*token_to_parse)++;

        node->left = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;
        node->right = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;

        return node;

    } else if (strcmp(token, "sin") == 0) {

        Node* node = get_node();
        node->operation = OP_SIN;

        (*token_to_parse)++;
        (*token_to_parse)++;

        node->left = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;
        node->right = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;

        return node;

    } else if (strcmp(token, "cos") == 0) {

        Node* node = get_node();
        node->operation = OP_COS;

        (*token_to_parse)++;
        (*token_to_parse)++;

        node->left = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;
        node->right = parse_tree(token_info, token_to_parse);

        (*token_to_parse)++;

        return node;

    } else if (strcmp(token, "x") == 0 || strcmp(token, "y") == 0 || isdigit(token[0]) || token[0] == '.' || token[0] == '-') {

        Node* node = get_node();

        if (strcmp(token, "x") == 0) {
            node->operation = OP_X;
        } else if (strcmp(token, "y") == 0) {
            node->operation = OP_Y;
        } else {
            node->operation = OP_VAL;
        }

        node->value = strdup(token);
        (*token_to_parse)++;

        return node;
    }

    return NULL;
}


float evaluate_tree(Node* node, float X, float Y) {

    if (node->operation == OP_VAL) {
        return atof(node->value);
    } else if (node->operation == OP_X) {
        return X;
    } else if (node->operation == OP_Y) {
        return Y;
    }

    float left, right;

    if (node->left != NULL) {
        left = evaluate_tree(node->left, X, Y);
    }

    if (node->right != NULL) {
        right = evaluate_tree(node->right, X, Y);
    }

    switch (node->operation) {

        case OP_ADD:
            return left + right;

        case OP_MUL:
            return left * right;

        case OP_SIN:
            return sinf(left + right);

        case OP_COS:
            return cosf(left * right);

        default:
            printf("UNREACHABLE\n");
            return 0.0;

    }

}

void free_tree(Node* node) {

    if(node == NULL) {
        return;
    }

    if(node->left) {
        free_tree(node->left);
    }

    if(node->right) {
        free_tree(node->right);
    }

    free(node);

}

float randf(float min, float max) {
    return (min + (float) rand() / (float)(RAND_MAX / (max - min)));
}

ImgGrid** map_to_img_grid(Node* parse_tree_root, char* motion_flag) {

    ImgGrid** img_grid = (ImgGrid**) malloc(sizeof(ImgGrid*) * (WIDTH));
    int rand_num = rand() % 11;

    for(int i = 0; i < WIDTH; i++) {

        img_grid[i] = (ImgGrid*) malloc(sizeof(ImgGrid) * HEIGHT);
        float ni = (float)i/WIDTH*2.0f - 1;

        for(int j = 0; j < HEIGHT; j++) {

            float nj = (float)j/HEIGHT*2.0f - 1;
            float eval = evaluate_tree(parse_tree_root, ni, nj);
            float scaled = (eval - 1)/2 * 255.0f;

            if (strcmp(motion_flag, "-p") == 0) {

                img_grid[i][j].c = (Color){
                        .r = scaled,
                        .g = scaled * (rand() % 11) * 4,
                        .b = scaled,
                        .a = 255,
                        // tweak's
                        // You can multiply r, g, b with random values to get different colors
                        // multiply with randf(start, end) to get distored pixel effect
                };

            } else if (strcmp(motion_flag, "-f2") == 0)  {
                
                img_grid[i][j].c = (Color){
                        .r = scaled * 3 * (randf(1, 2)),
                        .g = scaled * rand_num, // How many curves you want
                        .b = scaled,
                        .a = 255,
                        // tweak's
                        // You can multiply r, g, b with random values to get different colors
                        // multiply with randf(start, end) to get distored pixel effect
                };

            } else {

                img_grid[i][j].c = (Color){
                        .r = scaled * 3,
                        .g = scaled * 2, // How many curves you want
                        .b = scaled,
                        .a = 255,
                        // tweak's
                        // You can multiply r, g, b with random values to get different colors
                        // multiply with randf(start, end) to get distored pixel effect
                };

            }


        }
    }

    return img_grid;

}

void free_img_grid(ImgGrid** grid_img) {
    for (int i = 0; i < WIDTH; i++) {
        free(grid_img[i]);
    }
    free(grid_img);
}

void print_tree(Node* node) {

    if(node == NULL) {
        return;
    }

    printf("PARENT NODE: %s\n", get_op_name(node->operation));

    if (node->left != NULL) {
        printf("\tLEFT: %s\n", get_op_name(node->left->operation));
    } else if (node->left == NULL) {
        printf("\tLEFT: NULL\n");
    }

    if (node->right != NULL) {
        printf("\tRIGHT: %s\n", get_op_name(node->right->operation));
    } else if (node->right == NULL) {
        printf("\tRIGHT: NULL\n");
    }

    if(node->left) {
        print_tree(node->left);
    }

    if(node->right) {
        print_tree(node->right);
    }

}

int main(int argc, char** argv) {

    if(!argv[1]) {
        fprintf(stderr, "Usage: ./main <number_of_productions> <motion_flag>\n\n");
        fprintf(stderr, "Available options:\n");
        fprintf(stderr, "-f\t\tflowing motion\n");
        fprintf(stderr, "-f2\t\ttrailing red motion\n");
        fprintf(stderr, "-p\t\tParticle motion\n");
        exit(-1);
    }

    clock_t seed = clock();
    srand(seed);

    printf("SEED: %ld\n", seed);


    FILE* fp = fopen("./productions", "r");
    FILE* fp_output;

    Node* root;

    int token_to_parse = 0;
    int n = atoi(argv[1]); // total number of productions to include from the productions_file
    char* motion_flag = "";

    if(argv[2]) {
        motion_flag = argv[2];
    }

    if(!fp) {
        fprintf(stderr, "[!] ERROR READING FROM THE FILE 'Productions'\n");
        exit(-1);
    }

    Grammar g = read_productions(fp, n);
    int non_terminal = find_non_terminal(g, n, 'S');
    char* entry_point = get_random_production(g, non_terminal);
    generate_word(g, n, entry_point);

    fp_output = fopen("./production_output", "r");
    // tweak's
    // Change ./production_output to ./production_output2 to parse a string that you have generated using the grammar

    Token_Info* token_info = tokenize_expression(fp_output);
    root = parse_tree(*token_info, &token_to_parse);
    ImgGrid** img_grid = map_to_img_grid(root, motion_flag);

    // DISPLAY THE IMG
    
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "FRAME");
    SetTargetFPS(30);

    float amplitude = 0;
    float k = 0.05;
    float w = PI;
    float offset_r, offset_g, offset_b;
    float wave_time = 1.0f;

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int i = 0; i < WIDTH; i++) {
            for (int j = 0; j < HEIGHT; j++) {

                if (strcmp(motion_flag, "-f") == 0 || strcmp(motion_flag, "-f2") == 0) {

                    img_grid[i][j].c.r = (img_grid[i][j].c.r + 1) % 256;
                    img_grid[i][j].c.g = (img_grid[i][j].c.g + 2) % 256;
                    img_grid[i][j].c.b = (img_grid[i][j].c.b + 3) % 256;

                } else if (strcmp(motion_flag, "-p") == 0) {

                    offset_r = amplitude * sin((k * i) - (w * j) + wave_time + 1.0f);
                    offset_g = amplitude * cos((k * i) - (w * j) + wave_time + 2.0f);
                    offset_b = amplitude * tan((k * i) - (w * j) + wave_time + 3.5f);

                    int r = img_grid[i][j].c.r + offset_r * 7.29f;
                    int g = img_grid[i][j].c.g + offset_g * 7.71f;
                    int b = img_grid[i][j].c.b + offset_b * 7.73f;

                    img_grid[i][j].c.r = ((r % 256)) % 256;
                    img_grid[i][j].c.g = ((g % 256)) % 256;
                    img_grid[i][j].c.b = ((b % 256)) % 256;

                    if (wave_time == 1000000.0f) {
                        motion_flag = "-f";
                    } else if (amplitude > 2.0f) {
                        amplitude = 0.0f;
                    } else {
                        wave_time += 1.0f;
                        amplitude += 0.01f;
                    }

                }

                DrawPixel(i, j, img_grid[i][j].c);

            }

        }

        if (IsKeyPressed(KEY_S)) {
            TakeScreenshot("output.png");
            break;
        }

        EndDrawing();

    }

    CloseWindow();
    
    free(g);
    free(entry_point);
    fclose(fp);
    fclose(fp_output);
    free_tree(root);
    free_img_grid(img_grid);

    return 0;

}
