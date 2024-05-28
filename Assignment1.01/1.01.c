#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#define MIN(a,b) (((a)<(b))?(a):(b))

void createMap(char map[21][80]);
void roadMaker(char map[21][80]);
char boarderDecider(int choice);
void mapElement(char element, char map[21][80]);
bool placeCenter(bool placed, char type, int x, int y, char map[21][80]);
bool containAstrick(char map[21][80]);

int main(int argc, char *argv[])
{
    srand(time(NULL));
    char map[21][80];

    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            map[i][j] = '*';
        }
    }
    createMap(map);
    roadMaker(map);
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            printf("%c ", map[i][j]);
        }
        printf("\n");
    }
}

void createMap(char map[21][80])
{
    int tOrR;

   while(containAstrick)
    {
        mapElement('^', map);
        mapElement('.', map);
        mapElement(':', map);
        mapElement('%', map);
        mapElement('~', map);
        
    }

    for (int i = 0; i < 21; i++)
    {
        tOrR = rand() % 2;
        map[i][0] = boarderDecider(tOrR);
        map[i][79] = boarderDecider(tOrR);
    }
    for (int i = 0; i < 79; i++)
    {
        tOrR = rand() % 2;
        map[0][i] = boarderDecider(tOrR);
        map[20][i] = boarderDecider(tOrR);
    }
    return;
}
void roadMaker(char map[21][80])
{
    bool marketPlaced = false;
    bool comPlaced = false;

    int leftP = rand() % 19 + 1;
    int rightP = rand() % 19 + 1;
    int topP = rand() % 78 + 1;
    int bottomP = rand() % 78 + 1;
    int horMeet = rand() % 78 + 1;
    int verMeet = rand() % 19 + 1;
    for (int i = 0; i <= horMeet; i++)
    {
        map[leftP][i] = '#';
        if(i > 3){
         marketPlaced = placeCenter(marketPlaced, 'M', i, leftP, map);
        }
    }

    if (leftP < rightP)
        for (int i = leftP; i < rightP; i++)
        {
            map[i][horMeet] = '#';
        }

    else{
        for (int i = leftP; i > rightP; i--)
        {
            map[i][horMeet] = '#';
        }
    }
    for (int i = horMeet; i < 80; i++)
    {
        map[rightP][i] = '#';
        marketPlaced = placeCenter(marketPlaced, 'M', i, rightP, map);
    }

    for (int i = 0; i <= verMeet; i++)
    {
        map[i][topP] = '#';
        if(i > 3 && topP > 2){
            comPlaced = placeCenter(comPlaced, 'C', topP - 1, i, map);
        }
    }

    if (bottomP > topP)
        for (int i = topP; i < bottomP; i++)
        {
            map[verMeet][i] = '#';
        }
    else
        for (int i = topP; i > bottomP; i--)
        {
            map[verMeet][i] = '#';
        }

    for (int i = verMeet; i < 21; i++)
    {
        map[i][bottomP] = '#';
        comPlaced = placeCenter(comPlaced, 'C', bottomP - 1, i, map);
    }
    return;
}

char boarderDecider(int choice)
{
    if (choice == 1)
    {
        return '%';
    }
    else
    {
        return '^';
    }
}
void mapElement(char element, char map[21][80])
{
    int i = rand() % 20;
    int j = rand() % 75;
    int max = MIN(rand() % 18 + i, 21);
    int maxJ = MIN(j + rand() % 70, 80);
    for (int p = i; p < max; p++)
    {
        for (int k = j; k < maxJ; k++)
        {
            map[p][k] = element;
        }
    }
    return;
}
bool placeCenter(bool placed, char type, int x, int y, char map[21][80])
{
    if (!placed)
    {
        if (y > 2)
        {
            if (map[y-2][x] != ':' && map[y-2][x - 1] != ':' && map[y-1][x] != ':' && map[y-1][x-1] != ':' || map[y-2][x] != '#'
             && map[y-2][x - 1] != '#' && map[y-1][x] != '#' && map[y-1][x-1] != '#')
            {
                map[y- 2][x] = type;
                map[y - 2][x - 1] = type;
                map[y - 1][x] = type;
                map[y - 1][x - 1] = type;
                return true;
            }
        }
        else
        {
            if (map[y+2][x] != ':' && map[y + 2][x + 1] != ':' && map[y + 1][x] != ':' && map[y + 1][x + 1] != ':')
            {
                map[y + 2][x + 1] = type;
                map[y + 2][x] = type;
                map[y + 1][x + 1] = type;
                map[y + 1][x] = type;
                return true;
            }
        }
    }
    return true;
}
bool containAstrick(char map[21][80]){
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            if(map[i][j] == '*'){
                return true;
            }
        }
    }
    return false;
    
}