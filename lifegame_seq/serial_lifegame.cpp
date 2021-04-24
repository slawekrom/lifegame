#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include "EasyBMP.hpp"

const EasyBMP::RGBColor black(0, 0, 0);
const EasyBMP::RGBColor white(255, 255, 255);   
using namespace std;

int countNeighbors(int  **grid, int row, int col, int dimension);
void printGrid(int **grid, int dimension);
void copyGrid(int **gridAfterIteration, int** grid, int dimension);
void saveToFile(int **grid, int dimension, int iterNumber);

int main(int argc, char* argv[]){

	int dimension = atoi(argv[1]); // rozmiar macierzy - bok
	int iterationsNumer = atoi(argv[2]); // ile iteracji
	string initFilename = argv[3]; // plik początkowy - inicjalizacja
	bool saveOutput = atoi(argv[4]); // czy zapisywać wyjście do pliku

	//define colors
	// EasyBMP::RGBColor black(0, 0, 0);
	// EasyBMP::RGBColor white(255, 255, 255);    

	cout<<"window size "<< dimension<<" initFilename "<< initFilename<<endl;

	int** grid = new int*[dimension];

	for (int i = 0; i < dimension; i++) {
        grid[i] = new int[dimension];
    }

    int** gridAfterIteration = new int*[dimension];

	for (int i = 0; i < dimension; i++) {
        gridAfterIteration[i] = new int[dimension];
    }

    // load from file
    ifstream openfile(initFilename);
    if (!openfile.good()) {
    	cout << "Couldn't open the file.\n";
    	return 1;
	}
	else{
		while(!openfile.eof()){
			for (int i = 0; i < dimension; i++){
        		for (int j = 0; j < dimension; j++) {
            		int tmp;
            		openfile >> tmp;
            		grid[i][j] = tmp;
        		}
			}
		}
	}

	//print load data
	printGrid(grid, dimension);

    //iteration
	for (int k = 0; k < iterationsNumer; k++)
	{
		for (int i = 0; i < dimension; i++)
    {
        for (int j = 0; j < dimension; j++) {
            

            int neighbors = countNeighbors(grid, i, j, dimension);
            if (grid[i][j] == 0) // komórka niezamieszkana - 0
            {
            	// jeżeli pusta komórka ma 3 sąsiadów to staje się zamieszkana, pytanie czy też jeśli ma więej niż 3
            	gridAfterIteration[i][j] = (neighbors == 3 ? 1: 0);  
            }
            else{  // komórka zamieszkana - 1
            	if (neighbors >= 4) // 4 lub więcej - ginie
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
            }
        }
    }


    copyGrid(gridAfterIteration, grid, dimension);
    //print new grid
    //printGrid(gridAfterIteration, dimension);

    if (saveOutput)
    {
    	saveToFile(gridAfterIteration, dimension, k);
    }
	}

   	//free grid
    for (int i = 0; i < dimension; i++) {
        delete[] grid[i];
    }
    delete[] grid;

    for (int i = 0; i < dimension; i++) {
        delete[] gridAfterIteration[i];
    }
    delete[] gridAfterIteration;
	return 0;
}

int countNeighbors(int  **grid, int row, int col,int dimension){
	int count  = 0;

	for (int i = row - 1; i <= row + 1; i++)
	{
		for (int j = col - 1; j <= col + 1; j++)
		{
			if (i > -1 && i < dimension && j > -1 && j < dimension) // czy nie wyszliśmy poza tablice
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
void printGrid(int **grid, int dimension){
	for (int i = 0; i < dimension; i++)
    {
        for (int j = 0; j < dimension; j++) {
            cout << grid[i][j] << " ";
        }
        cout << endl;
    }
    cout<<endl;
}
void copyGrid(int **gridAfterIteration, int** grid, int dimension){
	for (int i = 0; i < dimension; i++)
    {
    	for (int j = 0; j < dimension; j++)
    	{
    		grid[i][j] = gridAfterIteration[i][j];
    	}
    }
}
void saveToFile(int **grid, int dimension, int iterNumber){
	ofstream openfile("output/" + to_string(iterNumber+1) + ".txt");

	EasyBMP::Image image(dimension, dimension, "img_out/sample" + to_string(iterNumber+1) + ".bmp", black);

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
            		image.SetPixel(i, j, tmp == 1 ? white : black);
        		}
        		openfile << "\n";
			}
	}
	image.Write();
	openfile.close();
}
