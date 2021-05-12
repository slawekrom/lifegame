#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <mpi.h>
#include <chrono>
#include "EasyBMP.hpp"

const EasyBMP::RGBColor black(0, 0, 0);
const EasyBMP::RGBColor white(255, 255, 255);   
using namespace std;

int countNeighbors(int  **grid, int row, int col, int dimension, int height);
void printGrid(int **grid, int height,int length, int startRow, int lastRow);
void copyGrid(int **gridAfterIteration, int** grid, int dimension, int height);
void saveToFile(int **grid, int dimension, int iterNumber);
void iterate(int **grid,int **gridAfterIteration, int dimension, int height, int startRow, int lastRow);
int countNeighborsForOneRow(int *rowGrid, int dimension, int col);
void allocateMemory(int dimension);
void freeMemory(int dimension);
void sendGrid(int **grid, int height, int length, int proc_rank);

int** matrix;
int* recievedRow;

int main(int argc, char* argv[]){

    int process_rank;
    int size;

    
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dimension = atoi(argv[1]); // rozmiar macierzy - bok
    int iterationsNumer = atoi(argv[2]); // ile iteracji
    string initFilename = argv[3]; // plik początkowy - inicjalizacja
    bool saveOutput = atoi(argv[4]); // czy zapisywać wyjście do pliku

    int process_rows_number = dimension / size;
    int firstCalcProcRank = 0;

    if (saveOutput)
    {
        if (size < 3) {
        printf("You need at least 3 processes");
        MPI_Finalize();
        exit(-1);
    }
    }
     else if (size < 2) {
        printf("You need at least 2 processes");
        MPI_Finalize();
        exit(-1);
    }

    if (saveOutput){
        firstCalcProcRank = 1;
        process_rows_number = dimension / (size-1);
        if (process_rank == 0){
            allocateMemory(dimension);
        }
    }
    // init grid for process
    int** grid = new int*[process_rows_number];

    int* rowUp = (int*)malloc(sizeof(int)*dimension);
    int* rowDown = (int*)malloc(sizeof(int)*dimension);

    for (int i = 0; i < process_rows_number; i++) {
        grid[i] = new int[dimension];
    }

    //init grid after iteration
    int** gridAfterIteration = new int*[process_rows_number];

    for (int i = 0; i < process_rows_number; i++) {
        gridAfterIteration[i] = new int[dimension];
    }

    ifstream openfile(initFilename);
    if (!openfile.good()) {
        cout << "Couldn't open the file.\n";
        return 1;
    }else{
        // printf("height %d\n", process_rows_number);
        if (saveOutput){
            if (process_rank > 0){
                openfile.seekg(((dimension * 2)) * (process_rank -1) * process_rows_number ,ios::beg);
            }
        }
        else{
            openfile.seekg(((dimension * 2)) * process_rank * process_rows_number ,ios::beg);  // przesunięcie o 1 linie = dlugosc*2
        }    
        int tmp;
        for (int i = 0; i < process_rows_number; i++)
        {
            for (int j = 0; j < dimension; j++)
            {
                openfile >> tmp;
                grid[i][j] = tmp;
            }
        }
        openfile.close();
    }

    // iteration
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int k = 0; k < iterationsNumer; k++){
        //jeśli zapis do pliku jest włączony to tworzymy proces nadrzędny, który będzie zbierał wyniki z procesów podrzędnych 
    if (saveOutput)
    {
        if (process_rank == 0)
        {
            MPI_Status Status;
            for (int i = 1; i < size; i++){
                
                for (int j = 0; j < process_rows_number; j++){
                    MPI_Recv(recievedRow, dimension, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &Status);
                    int rowNumber = Status.MPI_TAG;
                    memcpy(matrix[rowNumber], recievedRow, sizeof(int) * dimension);
                }
            }
	    printf("dupa 1");
            saveToFile(matrix, dimension, k);
            printf("no elo juz po");
	}
    }

    if (process_rank == firstCalcProcRank) // proces pierwszy
    {
        MPI_Request requests[2];
        MPI_Status status[2]; 
        //proces wysyła swój dolny wiersz do procesu "niżej" i  odbiera wirszy z wyższego procesu
        
        MPI_Isend(grid[process_rows_number-1], dimension, MPI_INT, process_rank+1, 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Irecv(rowDown, dimension, MPI_INT, process_rank+1, 0, MPI_COMM_WORLD, &requests[1]);
        
        //iteracja bez ostatniego wiersza
        iterate(grid, gridAfterIteration, dimension, process_rows_number, 0, 1);
        //printGrid(gridAfterIteration, process_rows_number, dimension, 0, 1); 

        MPI_Waitall(2,requests,status);
        // po odebraniu wiersza z niższego proceu liczymy ostatni wiersz
        for (int j = 0; j < dimension; j++)
        {
           int neighbors = countNeighbors(grid, process_rows_number-1, j, dimension, process_rows_number) + countNeighborsForOneRow(rowDown, dimension, j);
            if (grid[process_rows_number-1][j] == 0) // komórka niezamieszkana - 0
            {
                // jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
                gridAfterIteration[process_rows_number-1][j] = (neighbors == 3 ? 1: 0);  
            }else if (neighbors >= 4) // 4 lub więcej - ginie
            {
                gridAfterIteration[process_rows_number-1][j] = 0;
            }
            else if (neighbors < 2) // mniej niż 2 - ginie
            {
                gridAfterIteration[process_rows_number-1][j] = 0;
            }
            else{ // 2 lub 3 -- żyje 
                    gridAfterIteration[process_rows_number-1][j] = 1;
            }
        }
        // printf("\n");
        // printGrid(gridAfterIteration, process_rows_number, dimension, 0, 0); 

        copyGrid(gridAfterIteration, grid, dimension, process_rows_number);
        
        if (saveOutput){
            sendGrid(gridAfterIteration, process_rows_number, dimension, process_rank);
        }
    }
    else if (process_rank == size-1)  // proces ostatni
    {
        MPI_Request requests[2];
        MPI_Status status[2];
        //wysyła swój górny wiersz wyżej i  odbiera
        MPI_Isend(grid[0], dimension, MPI_INT, process_rank-1, 0, MPI_COMM_WORLD,  &requests[0]);
        MPI_Irecv(rowUp, dimension, MPI_INT, process_rank-1, 0, MPI_COMM_WORLD, &requests[1]);
            

        // iteracja bez pierwszego górnego wiersza
        iterate(grid, gridAfterIteration, dimension, process_rows_number, 1, 0);
        //printGrid(gridAfterIteration, process_rows_number, dimension, 1, 0); 

        MPI_Waitall(2,requests,status);
        // po odebraniu wiersza z wyższego proceu liczymy pierszy wiersz
        for (int j = 0; j < dimension; j++)
        {
            int neighbors = countNeighbors(grid, 0, j, dimension, process_rows_number) + countNeighborsForOneRow(rowUp, dimension, j);
            if (grid[0][j] == 0) // komórka niezamieszkana - 0
            {
                // jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
                gridAfterIteration[0][j] = (neighbors == 3 ? 1: 0);  
            }
            else if (neighbors >= 4) // 4 lub więcej - ginie
            {
                gridAfterIteration[0][j] = 0;
            }
            else if (neighbors < 2) // mniej niż 2 - ginie
            {
                gridAfterIteration[0][j] = 0;
            }
            else{ // 2 lub 3 -- żyje 
                gridAfterIteration[0][j] = 1;
            }
        }
        // printf("\n");
        // printGrid(gridAfterIteration, process_rows_number, dimension, 0, 0); 
        copyGrid(gridAfterIteration, grid, dimension, process_rows_number);
        if (saveOutput){
            sendGrid(gridAfterIteration, process_rows_number, dimension, process_rank);
        }


    }else{
        MPI_Request requests[4];
        MPI_Status status[4];
        //wysyła i dobiera z góry
        MPI_Isend(grid[0], dimension, MPI_INT, process_rank-1, 0, MPI_COMM_WORLD,  &requests[0]);
        MPI_Irecv(rowUp, dimension, MPI_INT, process_rank-1, 0, MPI_COMM_WORLD, &requests[1]);
        //wysyła i odbiera z dołu
        MPI_Isend(grid[process_rows_number-1], dimension, MPI_INT, process_rank+1, 0, MPI_COMM_WORLD,  &requests[2]);
        MPI_Irecv(rowDown, dimension, MPI_INT, process_rank+1, 0, MPI_COMM_WORLD, &requests[3]);   

        // iteracja "srodkowa"
        iterate(grid, gridAfterIteration, dimension, process_rows_number, 1, 1);
        MPI_Waitall(4,requests,status);
        // iteracja pierwszrgo i ostatniego  
        // po odebraniu wiersza z wyższego proceu liczymy pierszy wiersz
        for (int j = 0; j < dimension; j++)
        {
           int neighbors = countNeighbors(grid, 0, j, dimension, process_rows_number) + countNeighborsForOneRow(rowUp, dimension, j);
            if (grid[0][j] == 0) // komórka niezamieszkana - 0
            {
                // jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
                gridAfterIteration[0][j] = (neighbors == 3 ? 1: 0);  
            }
            else if (neighbors >= 4) // 4 lub więcej - ginie
            {
                gridAfterIteration[0][j] = 0;
            }
            else if (neighbors < 2) // mniej niż 2 - ginie
            {
                gridAfterIteration[0][j] = 0;
            }
            else{ // 2 lub 3 -- żyje 
                gridAfterIteration[0][j] = 1;
            }
        }

        // po odebraniu wiersza z niższego proceu liczymy ostatni wiersz
        for (int j = 0; j < dimension; j++)
        {
            int neighbors = countNeighbors(grid, process_rows_number-1, j, dimension, process_rows_number) + countNeighborsForOneRow(rowDown, dimension, j);
            if (grid[process_rows_number-1][j] == 0) // komórka niezamieszkana - 0
            {
                // jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
                gridAfterIteration[process_rows_number-1][j] = (neighbors == 3 ? 1: 0);  
            }
            else if (neighbors >= 4) // 4 lub więcej - ginie
            {
                gridAfterIteration[process_rows_number-1][j] = 0;
            }
            else if (neighbors < 2) // mniej niż 2 - ginie
            {
                gridAfterIteration[process_rows_number-1][j] = 0;
            }
            else{ // 2 lub 3 -- żyje 
                gridAfterIteration[process_rows_number-1][j] = 1;
            }
        }   
    }
    copyGrid(gridAfterIteration, grid, dimension, process_rows_number);
    if (saveOutput){
        sendGrid(gridAfterIteration, process_rows_number, dimension, process_rank);
    }
    }
    if (process_rank == 0){
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time = end_time - start_time;    
        cout<<"matrix size: "<<dimension<<" iterations: "<<iterationsNumer<<" time: "<<time/std::chrono::milliseconds(1)<<endl;          
    }

    for (int i = 0; i < dimension; i++) {
        delete[] grid[i];
    }
    delete[] grid;

    for (int i = 0; i < dimension; i++) {
        delete[] gridAfterIteration[i];
    }
    delete[] gridAfterIteration;
    delete[] rowDown;
    delete[] rowUp;
    if (saveOutput && process_rank == 0){
        freeMemory(dimension);
    }

     MPI_Finalize();
}

int countNeighbors(int  **grid, int row, int col,int dimension, int height){
	int count  = 0;

	for (int i = row - 1; i <= row + 1; i++)
	{
		for (int j = col - 1; j <= col + 1; j++)
		{
			if (i > -1 && i < height && j > -1 && j < dimension) // czy nie wyszliśmy poza tablice
			{
				if (grid[i][j] == 1)
				{
					count++;
				}
			}
		}
	}


	if (grid[row][col] ==1)
	{
		count--;
	}
	return count;
}

int countNeighborsForOneRow(int *rowGrid, int dimension, int col){
    int count = 0;
    for (int i = col - 1; i <= col + 1 ; i++)
    {
        if (i > -1 && i < dimension)
        {
            if (rowGrid[i] == 1)
            {
                count++;
            }
        }
    }
    return count;
}

void printGrid(int **grid, int height, int length, int startRow, int lastRow){
	for (int i = startRow; i < height - lastRow; i++)
    {
        for (int j = 0; j < length; j++) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}
void copyGrid(int **gridAfterIteration, int** grid, int dimension, int height){
	for (int i = 0; i < height; i++)
    {
    	for (int j = 0; j < dimension; j++)
    	{
    		grid[i][j] = gridAfterIteration[i][j];
    	}
    }
}
void saveToFile(int **grid, int dimension, int iterNumber){
    ofstream openfile("output/" + to_string(iterNumber+1) + ".txt");

    EasyBMP::Image image(dimension, dimension, "img_out/sample" + to_string(iterNumber+1) + ".bmp");

    if (!openfile.good()) {
        cout << "Couldn't open the file.\n";
        return;
    }
    else{
        for (int i = 0; i < dimension; i++){
                for (int j = 0; j < dimension; j++) {
                    int tmp = grid[i][j];
                    openfile << tmp;
                    openfile <<" ";
                    image.SetPixel(j, i, tmp == 1 ? white : black);
                }
                openfile << "\n";
            }
    }
    image.Write();
    openfile.close();
}

void iterate(int **grid, int **gridAfterIteration, int dimension, int height, int startRow, int lastRow){

    for (int i = startRow; i < height - lastRow; i++)
    {
        for (int j = 0; j < dimension; j++)
        {
            int neighbors = countNeighbors(grid, i, j, dimension, height);
            if (grid[i][j] == 0) // komórka niezamieszkana - 0
            {
                // jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
                gridAfterIteration[i][j] = (neighbors == 3 ? 1: 0);  
            }else if (neighbors >= 4) // 4 lub więcej - ginie
            {
                gridAfterIteration[i][j] = 0;
            }
            else if (neighbors < 2) // mniej niż 2 - ginie
            {
                gridAfterIteration[i][j] = 0;
            }
            else{ // 2 lub 3 -- żyje 
                gridAfterIteration[i][j] = 1;
                }
                //printf("grid %d \n", gridAfterIteration[i][j]);
        }
    }
}
void allocateMemory(int dimension){
    for (int i = 0; i < dimension; i++) {
        matrix[i] = new int[dimension];
    }
    recievedRow = (int*)malloc(sizeof(int)*dimension);
}
void freeMemory(int dimension){
    for (int i = 0; i < dimension; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}
void sendGrid(int **grid, int height, int length, int process_rank){
    for (int i = 0; i < height; i++)
    {
        MPI_Send(grid[i], length, MPI_INT, 0, (process_rank-1)  * height + i, MPI_COMM_WORLD);
    }
    delete[] recievedRow;
}
