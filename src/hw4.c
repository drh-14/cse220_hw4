#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT1 2201
#define PORT2 2202
#define BUFFER_SIZE 1024

#define EXPECTED_BEGIN_PACKET 100
#define EXPECTED_INITALIZE_PACKET 101
#define EXPECTED_SHOOT_QUERY_FORFEIT 102
#define INVALID_BEGIN_PACKET 200
#define INVALID_INITIALIZE_PACKET 201
#define INVALID_SHOOT_PACKET 202
#define SHAPE_OUT_OF_RANGE 300
#define ROTATION_OUT_OF_RANGE 301
#define INVALID_SHIP 302
#define OVERLAPPING_SHIPS 303
#define CELL_NOT_IN_BOARD 400
#define CELL_ALREADY_GUESSED 401


typedef struct{
    int height;
    int width;
    int **cells;
}Board;

 typedef struct{
  int coordinates[4][2];
}Piece;

typedef struct {
    Board board;
    Piece piece1;
    Piece piece2;
    Piece piece3;
    Piece piece4;
    Piece piece5;
    int remainingShips;
} Player;

void delete_board(Board board){
    for(int i = 0; i < board.height; i++){
        free(board.cells[i]);
    }
    free(board.cells);
}

int send_error(int target_fd, int ERROR_CODE){
    char errorMessage[20];
    snprintf(errorMessage, sizeof(errorMessage), "E %d", ERROR_CODE);
    return send(target_fd, errorMessage, strlen(errorMessage), 0);
}

bool piece_in_bounds(Board board, Piece piece){
    for(int i = 0; i < 4; i++){
        int row = piece.coordinates[i][0];
        int col = piece.coordinates[i][1];
        if(!(0 <= row && row < board.height && 0 <= col && col < board.width)){
            return false;
        }
    }
    return true;
}

bool ship_overlaps(Board board, Piece piece){
    int row, col;
    for(int i = 0; i < 4; i++){
        row, col = piece.coordinates[i][0], piece.coordinates[i][1];
        if(board.cells[row][col] != 0){
            return true;
        }
    }
   return false;
}

void build_piece(Piece piece, int pieceRotation, int pieceType, int row, int col){
    if(pieceType == 2){
        if(pieceRotation == 1 || pieceRotation == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row + 2, row + 3;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col, col;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row, row;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 2, col + 3;
        }
    }
    else if(pieceType == 3){
        if(pieceType == 1 || pieceType == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row - 1, row - 1;
            piece.coordinates[1][1], piece.coordinates[2][1] = piece.coordinates[3][1] = col + 1, col + 1, col + 2;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row + 1, row + 2;
            piece.coordinates[1][1], piece.coordinates[2][1] = piece.coordinates[3][1] = col, col + 1, col + 1;
        }
    }
    else if(pieceType == 4){
        if(pieceRotation == 1){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row + 2, row + 2;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col, col + 1;
        }
        else if(pieceRotation == 2){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row, row;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col + 1, col + 2;
        }
        else if(pieceRotation == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row + 1, row + 2;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 1;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row, row - 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 2, col + 2;
        }
    }
    else if(pieceType == 5){
        if(pieceRotation == 1 || pieceRotation == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row + 1, row + 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 2;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row, row - 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col + 1, col + 1;
        }
    }
    else if(pieceType == 6){
        if(pieceRotation == 1){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row - 1, row - 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 1;
        }
        else if(pieceRotation == 2){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row + 1, row + 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col + 1, col + 2;
        }
        else if(pieceRotation == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row + 1, row + 2;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col, col;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row, row + 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 2, col + 2;
        }
    }
    else if(pieceType == 7){
        if(pieceRotation == 1){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row + 1, row;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 2;
        }
        else if(pieceRotation == 2){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row - 1, row + 1;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 1;
        }
        else if(pieceRotation == 3){
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row, row - 1, row;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col + 1, col + 1, col + 2;
        }
        else{
            piece.coordinates[1][0], piece.coordinates[2][0], piece.coordinates[3][0] = row + 1, row + 1, row + 2;
            piece.coordinates[1][1], piece.coordinates[2][1], piece.coordinates[3][1] = col, col + 1, col;
        }
    }
}

bool hitShip(int row, int col, Piece piece){
    for(int i = 0; i < 4; i++){
        if(row == piece.coordinates[i][0] && col == piece.coordinates[i][1]){
            return true;
        }
    }
    return false;
}

void remove_ship(Player player, Piece piece){
    for(int i = 0; i < 4; i++){
        int row = piece.coordinates[i][0];
        int col = piece.coordinates[i][1];
        player.board.cells[row][col] = 0;
    }
    player.remainingShips--;
}

int shoot(Player player, int row, int col){
    if(hitShip(row, col, player.piece1)){
        remove_ship(player, player.piece1);
        return 1;
    }
    else if(hitShip(row, col, player.piece2)){
        remove_ship(player, player.piece2);
        return 2;
    }
    else if(hitShip(row, col, player.piece3)){
        remove_ship(player, player.piece3);
        return 3;
    }
    else if(hitShip(row, col, player.piece4)){
        remove_ship(player, player.piece4);
        return 4;
    }
    else if(hitShip(row, col, player.piece5)){
        remove_ship(player, player.piece5);
        return 5;
    }
  return -1;
}

int main(){
    int sock_fd_1, conn_fd_1;
    struct sockaddr_in address_1;
     int sock_fd_2, conn_fd_2;
    struct sockaddr_in address_2;
    int address_len_2 = sizeof(address_2);
    int address_len_1 = sizeof(address_1);
    char buffer[BUFFER_SIZE] = {0};
    sock_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd_1 == -1){
        perror("Socket failed.");
        exit(EXIT_FAILURE);
    }

     sock_fd_2 = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd_2 == -1){
        perror("Socket failed.");
        exit(EXIT_FAILURE);
    }
    address_1.sin_family = AF_INET;
    address_1.sin_port = htons(PORT1);
    address_1.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock_fd_1, (struct sockaddr*)&address_1, sizeof(address_1)) == -1){
        perror("Fail in bind.");
        exit(EXIT_FAILURE);
    }
    address_2.sin_family = AF_INET;
    address_2.sin_port = htons(PORT2);
    address_2.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock_fd_2, (struct sockaddr*)&address_2, sizeof(address_2)) == -1){
        perror("Failed to bind.");
        exit(EXIT_FAILURE);
    }
    if(listen(sock_fd_1, 3) == -1){
        perror("Failed to listen.");
        exit(EXIT_FAILURE);
    }

     printf("[Server] running on port %d\n", PORT1);
    conn_fd_1 = accept(sock_fd_1, (struct sockaddr*)&address_1, (socklen_t *)&address_len_1);
    if(conn_fd_1 == -1){
        perror("Failed to accept.");
        exit(EXIT_FAILURE);
    }

    if(listen(sock_fd_2, 3) == -1){
        perror("Failed to listen.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] running on port %d\n", PORT2);
    conn_fd_2 = accept(sock_fd_2, (struct sockaddr*)&address_2, (socklen_t *)&address_len_2);
    if(conn_fd_2 == -1){
        perror("Failed to accept.");
        exit(EXIT_FAILURE);
    }

    int height, width;
    while(1){
    int numBytes = read(conn_fd_1, buffer, BUFFER_SIZE);
        if (numBytes <= 0) {
            continue;
        }
        if(buffer[0] == 'F'){
            char message1[15];
            snprintf(message1, sizeof(message1), "H %d", 0);
            send(conn_fd_1, message1, strlen(message1), 0);
            char message2[15];
            snprintf(message2, sizeof(message2), "H %d", 1);
            send(conn_fd_2, message2, strlen(message2), 0);
            memset(buffer, 0, BUFFER_SIZE);
            close(sock_fd_1);
            close(sock_fd_2);
            return 0;
        }
        char *tokens = strtok(buffer, " ");
        if(*tokens != 'B'){
            send_error(conn_fd_1, EXPECTED_BEGIN_PACKET);
            memset(buffer, 0, BUFFER_SIZE);
            continue;
        }
        if(strlen(tokens) != 3){
            send_error(conn_fd_1, INVALID_BEGIN_PACKET);
            memset(buffer, 0, BUFFER_SIZE);
            continue;
        }
        width = (int)tokens[1];
        height = (int)tokens[2];
        if(width < 10 || height < 10){
            send_error(conn_fd_1, INVALID_BEGIN_PACKET);
            memset(buffer, 0,  BUFFER_SIZE);
            continue;
        }
        send(conn_fd_1, "A", 1, 0);
        break;
    }
    
    // Initialize Player 1 board
     Player player1;
    player1.board.height = height;
    player1.board.width = width;
    player1.board.cells = malloc(sizeof(int*) * height);
    for(int i = 0; i < height; i++){
        player1.board.cells[i] = malloc(sizeof(int) * width);
    }
   memset(buffer, 0, BUFFER_SIZE);

   // Player 2
   while(1){ 
   int numBytes = read(conn_fd_2, buffer, BUFFER_SIZE);
        if (numBytes <= 0) {
            continue;
        }
        if(buffer[0] == 'F'){
            char message1[15];
            snprintf(message1, sizeof(message1), "H %d\n", 1);
            send(conn_fd_1, message1, strlen(message1), 0);
            char message2[15];
            snprintf(message2, sizeof(message2), "H %d\n", 0);
            send(conn_fd_2, message2, strlen(message2), 0);
            memset(buffer, 0, BUFFER_SIZE);
            close(sock_fd_1);
            close(sock_fd_2);
            return 0;
        }
        if(buffer[0] != 'B'){
            send_error(conn_fd_2, EXPECTED_BEGIN_PACKET);
            continue;
        }
        if(numBytes != 1){
            send_error(conn_fd_2, INVALID_BEGIN_PACKET);
        }
        break;
   }
    Player player2;
    player2.board.height = height;
    player2.board.width = width;
    player2.board.cells = malloc(sizeof(int*) * height);
    for(int i = 0; i < height; i++){
        player2.board.cells[i] = malloc(sizeof(int) * width);
    }
    memset(buffer, 0, BUFFER_SIZE);

    // Initialize Packets

    // Player 1

    while(1){
        int numBytes = read(conn_fd_1, buffer, BUFFER_SIZE);
        if(numBytes <= 0){
            continue;
        }
        if(buffer[0] == 'F'){
            // Handle forfeit logic
        }
        if(buffer[0] != 'I'){
            send_error(conn_fd_1, EXPECTED_INITALIZE_PACKET);
            memset(buffer, 0, BUFFER_SIZE);
            continue;
        }
        char *tokens = strtok(buffer, " ");
        if(strlen(tokens) != 21){
            send_error(conn_fd_1, INVALID_INITIALIZE_PACKET);
            memset(buffer, 0, BUFFER_SIZE);
            continue;
        }
        tokens++;
        Piece pieces[5];
        bool invalidPieces = true;
        int pieceType, pieceRotation, row, col;
        for(int i = 0; i < 4; i++){
            pieceType = 5 * i;
            pieceRotation = 5 * i + 1;
            row = 5 * i + 2;
            col  = 5 * i + 3;
            if(!(1 <= pieceType && pieceType <= 7)){
                send_error(conn_fd_1, SHAPE_OUT_OF_RANGE);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            if(!(1 <= pieceRotation && pieceRotation <= 4)){
                send_error(conn_fd_1, ROTATION_OUT_OF_RANGE);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            Piece piece;
            build_piece(piece, pieceRotation, pieceType, row, col);
            if(!piece_in_bounds(player1.board, piece)){
                send_error(conn_fd_1, INVALID_SHIP);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            if(ship_overlaps(player1.board, piece)){
                send_error(conn_fd_1, OVERLAPPING_SHIPS);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            for(int i = 0; i < 4; i++){
                int row = piece.coordinates[i][0];
                int col = piece.coordinates[i][1];
                player1.board.cells[row][col] = 1;
            }
            pieces[i] = piece;
            invalidPieces = false;
        }
        if(invalidPieces){
            continue;
        }
        else{
            player1.piece1 = pieces[0];
            player1.piece2 = pieces[1];
            player1.piece3 = pieces[2];
            player1.piece4 = pieces[3];
            player1.piece5 = pieces[4];
            player1.remainingShips = 5;
            memset(buffer, 0, BUFFER_SIZE);
            break;
        }
    }

    // Player 2

    while(1){
        int numBytes = read(conn_fd_2, buffer, BUFFER_SIZE);
        if(numBytes <= 0){
            send(conn_fd_2, "A", 1, 0);
            continue;
        }
        if(buffer[0] == 'F'){
            // Handle forfeit logic
        }
        if(buffer[0] != 'I'){
            send_error(conn_fd_2, EXPECTED_INITALIZE_PACKET);
            continue;
        }
        char *tokens = strtok(buffer, " ");
        if(strlen(tokens) != 21){
            send_error(conn_fd_2, INVALID_INITIALIZE_PACKET);
        }
        tokens++;
        Piece pieces[5];
        bool invalidPieces = true;
        int pieceType, pieceRotation, row, col;
        for(int i = 0; i < 4; i++){
            pieceType = 5 * i;
            pieceRotation = 5 * i + 1;
            row = 5 * i + 2;
            col  = 5 * i + 3;
            if(!(1 <= pieceType && pieceType <= 7)){
                send_error(conn_fd_2, SHAPE_OUT_OF_RANGE);
                break;
            }
            if(!(1 <= pieceRotation && pieceRotation <= 7)){
                send_error(conn_fd_2, ROTATION_OUT_OF_RANGE);
                break;
            }
            Piece piece;
            build_piece(piece, pieceRotation, pieceType, row, col);
            if(!piece_in_bounds(player2.board, piece)){
                send_error(conn_fd_2, INVALID_SHIP);
                continue;
            }
            if(ship_overlaps(player2.board, piece)){
                send_error(conn_fd_2, OVERLAPPING_SHIPS);
                break;
            }
            for(int i = 0; i < 4; i++){
                int row = piece.coordinates[i][0];
                int col = piece.coordinates[i][1];
                player2.board.cells[row][col] = 1;
            }
            pieces[i] = piece;
            invalidPieces = false;
        }
        if(invalidPieces){
            continue;
        }
        else{
            player2.piece1 = pieces[0];
            player2.piece2 = pieces[1];
            player2.piece3 = pieces[2];
            player2.piece4 = pieces[3];
            player2.piece5 = pieces[4];
            player2.remainingShips = 5;
            break;
        }
    }

    // Shoot / Query / Forfeit Packets

    // Player 1

    while(1){

    }

    
  // Free allocated memory and close server

  delete_board(player1.board);
  delete_board(player2.board);
  close(sock_fd_1);
  close(sock_fd_2);
  printf("%s", "[Server] shutting down.");
  return 0;
}